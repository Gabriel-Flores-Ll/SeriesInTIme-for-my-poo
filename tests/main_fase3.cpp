#include "ChannelSignal.h"
#include "DetectecEvent.h"
#include "DetectionEngine.h"
#include "EventDetector.h"
#include "EventRegistry.h"
#include "PeakDetector.h"
#include "PeakEvent.h"
#include "StatisticalAnomalyEvent.h"
#include "ThresholdDetector.h"
#include "ThresholdEvent.h"
#include "WindowAnomalyDetector.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

int gPassed = 0;
int gFailed = 0;

bool approxEqual(double a, double b, double tol = 1e-6) {
    return std::fabs(a - b) < tol;
}

void check(bool condition, const char* label) {
    if (condition) {
        ++gPassed;
    } else {
        ++gFailed;
        std::cerr << "[FAIL] " << label << "\n";
    }
}

template <typename Fn>
void expectInvalidArg(Fn fn, const char* label) {
    try {
        fn();
        ++gFailed;
        std::cerr << "[FAIL] Expected std::invalid_argument: " << label << "\n";
    } catch (const std::invalid_argument&) {
        ++gPassed;
    } catch (...) {
        ++gFailed;
        std::cerr << "[FAIL] Wrong exception (expected invalid_argument): " << label << "\n";
    }
}

template <typename Fn>
void expectOutOfRange(Fn fn, const char* label) {
    try {
        fn();
        ++gFailed;
        std::cerr << "[FAIL] Expected std::out_of_range: " << label << "\n";
    } catch (const std::out_of_range&) {
        ++gPassed;
    } catch (...) {
        ++gFailed;
        std::cerr << "[FAIL] Wrong exception (expected out_of_range): " << label << "\n";
    }
}

void testEventInvariantsAndOperators() {
    const double kNan = std::numeric_limits<double>::quiet_NaN();
    const double kInf = std::numeric_limits<double>::infinity();

    expectInvalidArg([]() { PeakEvent("", "mV", 0, 0.0, 1.0, 1.0, 0.5, true); },
                     "DetectedEvent rejects empty channel");
    expectInvalidArg([]() { PeakEvent("CH1", "", 0, 0.0, 1.0, 1.0, 0.5, true); },
                     "DetectedEvent rejects empty unit");
    expectInvalidArg([kInf]() { PeakEvent("CH1", "mV", 0, kInf, 1.0, 1.0, 0.5, true); },
                     "DetectedEvent rejects +Inf timestamp");
    expectInvalidArg([kNan]() { PeakEvent("CH1", "mV", 0, 0.0, kNan, 1.0, 0.5, true); },
                     "DetectedEvent rejects NaN severity");
    expectInvalidArg([]() {
        ThresholdEvent("CH1", "mV", 5, 4, 0.5, 0.6, 1.0, 2.0, 3.0, ThresholdEvent::CrossingType::UPPER);
    }, "DetectedEvent rejects endIndex < initIndex");
    expectInvalidArg([]() {
        ThresholdEvent("CH1", "mV", 2, 4, 0.2, 0.4, 1.0, 2.0, 1.5, ThresholdEvent::CrossingType::UPPER);
    }, "ThresholdEvent UPPER rejects extreme < threshold");
    expectInvalidArg([]() {
        ThresholdEvent("CH1", "mV", 2, 4, 0.2, 0.4, 1.0, -2.0, -1.5, ThresholdEvent::CrossingType::LOWER);
    }, "ThresholdEvent LOWER rejects extreme > threshold");
    expectInvalidArg([]() {
        StatisticalAnomalyEvent("CH1", "mV", 0, 3, 0.0, 0.3, 2.0, 2.0, 1.0, 0.0, 1.0, 0);
    }, "StatisticalAnomalyEvent rejects involvedWindows < 1");

    PeakEvent p1("CH1", "mV", 10, 1.0, 3.0, 5.0, 2.0, true);
    PeakEvent p2("CH1", "mV", 10, 1.0, 4.0, 5.0, 2.0, true);
    check(p2 < p1 && p1 != p2, "operator< and operator== remain consistent when only severity differs");
}

void testEventRegistryRuleOfThreeAndQueries() {
    EventRegistry reg(0);
    reg += PeakEvent("CH1", "mV", 15, 1.5, 2.0, 3.0, 1.0, true);
    reg += ThresholdEvent("CH1", "mV", 4, 8, 0.4, 0.8, 5.0, 1.0, 3.5, ThresholdEvent::CrossingType::UPPER);
    reg += StatisticalAnomalyEvent("CH2", "uV", 10, 14, 1.0, 1.4, 3.5, -3.5, 4.0, 0.0, 1.0, 2);

    EventRegistry copy(reg);
    check(copy == reg && &copy[0] != &reg[0], "EventRegistry copy constructor deep-copies");

    EventRegistry assigned(1);
    assigned += PeakEvent("OLD", "V", 0, 0.0, 0.5, 1.0, 0.5, true);
    assigned = reg;
    EventRegistry& alias = assigned;
    assigned = alias;
    check(assigned == reg, "EventRegistry assignment and self-assignment preserve state");

    copy += copy;
    check(copy.getCount() == 6 && copy[0] == copy[3] && copy[2] == copy[5],
          "EventRegistry self-concatenation (reg += reg) is safe");

    EventRegistry combined = reg + reg;
    check(combined.getCount() == 6 && reg.getCount() == 3, "EventRegistry operator+ does not mutate operands");

    reg.sortChronologically();
    check(reg[0].getInitIndex() == 4 && reg[1].getInitIndex() == 10 && reg[2].getInitIndex() == 15,
          "EventRegistry sortChronologically");

    reg.sortBySeverity();
    check(approxEqual(reg[0].getSeverity(), 5.0) &&
          approxEqual(reg[1].getSeverity(), 3.5) &&
          approxEqual(reg[2].getSeverity(), 2.0),
          "EventRegistry sortBySeverity");

    EventRegistry overlap = reg.filterByTimeRange(0.7, 1.1);
    check(overlap.getCount() == 2 &&
          overlap.countByType("THRESHOLD") == 1 &&
          overlap.countByType("STATISTICAL_ANOMALY") == 1,
          "EventRegistry filterByTimeRange captures overlapping intervals");

    DetectedEvent* ownedPtr = new PeakEvent("CH1", "mV", 2, 0.2, 1.0, 1.5, 0.5, true);
    reg.adopt(ownedPtr);
    expectInvalidArg([&reg, ownedPtr]() { reg.adopt(ownedPtr); },
                     "EventRegistry adopt rejects already-owned pointer safely");
    expectOutOfRange([&reg]() { (void)reg[99]; }, "EventRegistry operator[] bounds-checks");
}

void testPeakDetector() {
    expectInvalidArg([]() { PeakDetector("", 1.0, 1.0); }, "PeakDetector rejects empty name");
    expectInvalidArg([]() { PeakDetector("pk", 1.0, 1.0, 1, false, "invalid"); },
                     "PeakDetector rejects invalid buffer selector");
    expectInvalidArg([]() { PeakDetector("pk", -1.0, 1.0); }, "PeakDetector rejects negative minAmplitude");
    expectInvalidArg([]() { PeakDetector("pk", 1.0, -0.5); }, "PeakDetector rejects negative minProminence");
    expectInvalidArg([]() { PeakDetector("pk", 1.0, 1.0, 0); }, "PeakDetector rejects minDistanceSamples < 1");

    const double samples[15] = {
        0.0, 1.0, 5.0, 1.0, 0.0, 4.0, 8.0, 8.0, 2.0, 0.0, -6.0, 0.0, 3.0, 1.0, 0.0
    };
    ChannelSignal sig("ECG1", "mV", 100.0, samples, 15);
    const double mean = sig.getProcessedStatistics().getAverage();
    const double stdDev = sig.getProcessedStatistics().getStandardDesviation();

    // Positive peaks only (includes flat-top plateau at indices 6-7 reported at index 6)
    PeakDetector detPos("pos_peaks", 2.0, 1.0, 1, false, "processed");
    EventRegistry posRes = detPos.detect(sig);
    check(posRes.getCount() == 3, "PeakDetector finds 3 positive peaks including flat-top plateau");
    check(posRes[0].getInitIndex() == 2 && posRes[1].getInitIndex() == 6 && posRes[2].getInitIndex() == 12,
          "PeakDetector reports exact peak indices (2, 6, 12)");
    const double expectedSev6 = std::fabs(8.0 - mean) / stdDev;
    check(approxEqual(posRes[1].getSeverity(), expectedSev6),
          "PeakDetector normalizes severity by baseline stdDev");

    // Refractory distance suppresses index 2 in favor of higher-prominence index 6 (6 - 2 = 4 < 5)
    PeakDetector detRef("refractory_peaks", 2.0, 1.0, 5, false, "processed");
    EventRegistry refRes = detRef.detect(sig);
    check(refRes.getCount() == 2 && refRes[0].getInitIndex() == 6 && refRes[1].getInitIndex() == 12,
          "PeakDetector refractory suppression retains stronger peak in cluster");

    // Positive peaks + negative valleys
    PeakDetector detVal("peaks_and_valleys", 2.0, 2.0, 1, true, "processed");
    EventRegistry valRes = detVal.detect(sig);
    check(valRes.getCount() == 3 && valRes[2].getInitIndex() == 10,
          "PeakDetector detects negative valley at index 10");
    const auto* valleyPtr = dynamic_cast<const PeakEvent*>(&valRes[2]);
    check(valleyPtr != nullptr && !valleyPtr->isPositivePeak() && approxEqual(valleyPtr->getPeakAmplitude(), -6.0),
          "PeakDetector marks valley polarity as false with exact amplitude");

    // Buffer selector differentiation ("processed" vs "raw")
    sig.setProcessedElement(0.0, 2);
    EventRegistry procAfterEdit = detPos.detect(sig);
    PeakDetector detRaw("raw_peaks", 2.0, 1.0, 1, false, "raw");
    EventRegistry rawAfterEdit = detRaw.detect(sig);
    check(procAfterEdit.getCount() == 2 && rawAfterEdit.getCount() == 3,
          "PeakDetector respects processed vs raw buffer selection");

    // Short signal (N < 3) and flat signal produce 0 peaks
    const double shortBuf[2] = {1.0, 9.0};
    ChannelSignal shortSig("S2", "mV", 10.0, shortBuf, 2);
    check(detPos.detect(shortSig).isEmpty(), "PeakDetector returns empty on N < 3");

    const double flatBuf[6] = {5.0, 5.0, 5.0, 5.0, 5.0, 5.0};
    ChannelSignal flatSig("FLAT", "mV", 10.0, flatBuf, 6);
    check(detPos.detect(flatSig).isEmpty(), "PeakDetector returns empty on flat signal");
}

void testThresholdDetector() {
    expectInvalidArg([]() {
        ThresholdDetector("t", 3.0, 4.0, 1, ThresholdEvent::CrossingType::UPPER);
    }, "ThresholdDetector UPPER rejects release > activation");
    expectInvalidArg([]() {
        ThresholdDetector("t", -3.0, -4.0, 1, ThresholdEvent::CrossingType::LOWER);
    }, "ThresholdDetector LOWER rejects release < activation");
    expectInvalidArg([]() {
        ThresholdDetector("t", 3.0, 2.0, 0, ThresholdEvent::CrossingType::UPPER);
    }, "ThresholdDetector rejects minDurationSamples < 1");

    const double samples[14] = {
        0.0, 2.5, 4.0, 3.2, 5.5, 1.5, 0.0, 4.5, 0.0, -4.0, -5.0, -2.5, -3.5, -6.0
    };
    ChannelSignal sig("EMG1", "mV", 10.0, samples, 14);
    const double stdDev = sig.getProcessedStatistics().getStandardDesviation();

    // UPPER hysteresis: [2,4] survives dip at index 3 (3.2 >= 2.0); single-sample spike at index 7 dropped (< minDur 2)
    ThresholdDetector detUp("thr_up", 3.5, 2.0, 2, ThresholdEvent::CrossingType::UPPER, "processed");
    EventRegistry upRes = detUp.detect(sig);
    check(upRes.getCount() == 1, "ThresholdDetector UPPER emits 1 sustained excursion and filters short spike");
    check(upRes[0].getInitIndex() == 2 && upRes[0].getEndIndex() == 4 &&
          approxEqual(upRes[0].getCharacteristicValue(), 5.5) &&
          approxEqual(upRes[0].getSeverity(), std::fabs(5.5 - 3.5) / stdDev),
          "ThresholdDetector UPPER captures hysteresis interval, extreme, and normalized severity");

    // LOWER hysteresis + flush open excursion at end of signal: [9,13], extreme = -6.0
    ThresholdDetector detLow("thr_low", -3.5, -2.0, 2, ThresholdEvent::CrossingType::LOWER, "processed");
    EventRegistry lowRes = detLow.detect(sig);
    check(lowRes.getCount() == 1, "ThresholdDetector LOWER flushes active excursion at end of signal");
    check(lowRes[0].getInitIndex() == 9 && lowRes[0].getEndIndex() == 13 &&
          approxEqual(lowRes[0].getCharacteristicValue(), -6.0) &&
          approxEqual(lowRes[0].getSeverity(), std::fabs(-6.0 - (-3.5)) / stdDev),
          "ThresholdDetector LOWER captures full interval [9,13] and extreme -6.0");

    // Flat signal exceeding threshold (stdDev == 0 < 1e-12): fallback divisor 1.0 avoids NaN/Inf and preserves excess
    const double flatOver[5] = {10.0, 10.0, 10.0, 10.0, 10.0};
    ChannelSignal flatSig("SAT1", "V", 10.0, flatOver, 5);
    ThresholdDetector detFlat("thr_flat", 5.0, 4.0, 1, ThresholdEvent::CrossingType::UPPER, "processed");
    EventRegistry flatRes = detFlat.detect(flatSig);
    check(flatRes.getCount() == 1 && flatRes[0].getInitIndex() == 0 && flatRes[0].getEndIndex() == 4 &&
          approxEqual(flatRes[0].getSeverity(), 5.0),
          "ThresholdDetector handles flat overload (stdDev==0) with fallback scale 1.0");
}

void testWindowAnomalyDetector() {
    expectInvalidArg([]() { WindowAnomalyDetector("w", 0, 1); }, "WindowAnomalyDetector rejects windowSize < 1");
    expectInvalidArg([]() { WindowAnomalyDetector("w", 4, 0); }, "WindowAnomalyDetector rejects stride < 1");
    expectInvalidArg([]() { WindowAnomalyDetector("w", 4, 2, 0.0, 2.0); },
                     "WindowAnomalyDetector rejects non-positive zScoreThreshold");
    expectInvalidArg([]() { WindowAnomalyDetector("w", 4, 2, 2.0, -1.0); },
                     "WindowAnomalyDetector rejects non-positive energyRatioThreshold");

    // 40-sample signal with two disjoint anomalous episodes:
    //   1) [10..15] DC jump +10.0 (merges windows [10..13] and [12..15])
    //   2) [28..31] zero-mean high-energy burst (+8/-8/+8/-8) triggering energyRatio
    double buf[40];
    for (int i = 0; i < 40; ++i) {
        buf[i] = (i % 2 == 0) ? 1.0 : -1.0;
    }
    for (int i = 10; i <= 15; ++i) {
        buf[i] = 10.0;
    }
    buf[28] = 8.0;
    buf[29] = -8.0;
    buf[30] = 8.0;
    buf[31] = -8.0;

    ChannelSignal sig("EEG1", "uV", 10.0, buf, 40);
    WindowAnomalyDetector det("win_anom", 4, 2, 1.8, 2.5, "processed");
    EventRegistry res = det.detect(sig);

    check(res.getCount() == 2, "WindowAnomalyDetector detects 2 disjoint anomalous episodes");

    const auto* ep1 = dynamic_cast<const StatisticalAnomalyEvent*>(&res[0]);
    const auto* ep2 = dynamic_cast<const StatisticalAnomalyEvent*>(&res[1]);
    check(ep1 != nullptr && ep1->getInitIndex() == 10 && ep1->getEndIndex() == 15 &&
          ep1->getInvolvedWindows() == 2,
          "WindowAnomalyDetector merges overlapping anomalous windows [10..13] and [12..15]");
    check(ep1 != nullptr &&
          approxEqual(ep1->getSeverity(), std::sqrt(ep1->getEnergyRatio())),
          "WindowAnomalyDetector computes linear severity max(|Z|, sqrt(R_E)) on energy-dominant episode");

    check(ep2 != nullptr && ep2->getInitIndex() == 28 && ep2->getEndIndex() == 31 &&
          ep2->getInvolvedWindows() == 1 && ep2->getEnergyRatio() >= 2.5,
          "WindowAnomalyDetector flags zero-mean high-energy burst via energy ratio");

    // Pure negative Z-score drop (energyRatio == 0.0 -> |Z| dominates severity)
    double dropBuf[40];
    for (int i = 0; i < 40; ++i) {
        dropBuf[i] = (i % 2 == 0) ? 10.1 : 9.9;
    }
    for (int i = 20; i <= 23; ++i) {
        dropBuf[i] = 0.0;
    }
    ChannelSignal dropSig("FLOW1", "L/m", 10.0, dropBuf, 40);
    WindowAnomalyDetector detDrop("win_drop", 4, 4, 2.5, 2.5, "processed");
    EventRegistry dropRes = detDrop.detect(dropSig);
    check(dropRes.getCount() == 1, "WindowAnomalyDetector detects pure Z-score dropout");
    const auto* dropEv = dynamic_cast<const StatisticalAnomalyEvent*>(&dropRes[0]);
    check(dropEv != nullptr && dropEv->getMaxZScore() < -2.5 &&
          approxEqual(dropEv->getEnergyRatio(), 0.0) &&
          approxEqual(dropEv->getSeverity(), std::fabs(dropEv->getMaxZScore())),
          "WindowAnomalyDetector preserves signed negative Z-score and sets severity = |Z| when energyRatio == 0");

    // Signal shorter than windowSize and completely flat signal return empty safely
    const double tiny[3] = {1.0, 2.0, 3.0};
    ChannelSignal tinySig("TINY", "mV", 10.0, tiny, 3);
    check(det.detect(tinySig).isEmpty(), "WindowAnomalyDetector returns empty when N < windowSize");

    const double zeros[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    ChannelSignal zeroSig("ZERO", "mV", 10.0, zeros, 10);
    check(det.detect(zeroSig).isEmpty(), "WindowAnomalyDetector handles zero-energy flat signal without NaN");
}

void testDetectionEngineAndLifetimeIndependence() {
    expectInvalidArg([]() { DetectionEngine bad(-1); }, "DetectionEngine rejects negative capacity");

    DetectionEngine engine(1);
    engine.addDetector(PeakDetector("ecg_peaks", 3.0, 2.0, 2, false, "processed"));
    engine.addDetector(ThresholdDetector("ecg_thr", 4.0, 2.5, 2, ThresholdEvent::CrossingType::UPPER, "processed"));

    EventDetector* adoptedDet = new WindowAnomalyDetector("ecg_win", 4, 2, 1.8, 2.5, "processed");
    engine.adoptDetector(adoptedDet);

    expectInvalidArg([&engine]() { engine.adoptDetector(nullptr); },
                     "DetectionEngine adoptDetector rejects nullptr");
    expectInvalidArg([&engine, adoptedDet]() { engine.adoptDetector(adoptedDet); },
                     "DetectionEngine adoptDetector rejects duplicate pointer safely");
    expectOutOfRange([&engine]() { (void)engine.getDetector(3); },
                     "DetectionEngine getDetector bounds-checks");

    // Rule of Three on DetectionEngine (copy ctor, copy assign, self-assign)
    DetectionEngine copyEngine(engine);
    check(copyEngine.getDetectorCount() == 3 &&
          &copyEngine.getDetector(0) != &engine.getDetector(0),
          "DetectionEngine copy constructor deep-clones detectors");

    DetectionEngine assignedEngine(0);
    assignedEngine = copyEngine;
    DetectionEngine& selfRef = assignedEngine;
    assignedEngine = selfRef;
    check(assignedEngine.getDetectorCount() == 3, "DetectionEngine assignment and self-assignment preserve count");

    // Run on multiple heterogeneous channels inside an inner scope, then destroy signals to verify 0:0 decoupling
    EventRegistry multiChrono(0);
    EventRegistry multiSev(0);
    {
        double sig1Data[20];
        double sig2Data[20];
        for (int i = 0; i < 20; ++i) {
            sig1Data[i] = (i % 2 == 0) ? 0.5 : -0.5;
            sig2Data[i] = (i % 2 == 0) ? 1.0 : -1.0;
        }
        sig1Data[6] = 6.0;
        sig1Data[7] = 5.0;
        sig2Data[12] = 9.0;
        sig2Data[13] = 8.0;

        ChannelSignal ch1("ECG_A", "mV", 100.0, sig1Data, 20);
        ChannelSignal ch2("ACC_Z", "g", 50.0, sig2Data, 20);
        const ChannelSignal* channels[2] = {&ch1, &ch2};

        multiChrono = assignedEngine.runMultiple(channels, 2, false);
        multiSev = assignedEngine.runMultiple(channels, 2, true);

        const ChannelSignal* badChannels[1] = {nullptr};
        expectInvalidArg([&assignedEngine]() { (void)assignedEngine.runMultiple(nullptr, 1); },
                         "DetectionEngine runMultiple rejects null array with count > 0");
        expectInvalidArg([&assignedEngine, badChannels]() { (void)assignedEngine.runMultiple(badChannels, 1); },
                         "DetectionEngine runMultiple rejects null channel entry");
        expectInvalidArg([&assignedEngine]() { (void)assignedEngine.runMultiple(nullptr, -1); },
                         "DetectionEngine runMultiple rejects negative signalCount");
    }
    // Both ChannelSignal objects are now destroyed; registries must remain completely valid (zero dangling pointers)
    check(multiChrono.getCount() > 0 && multiChrono.getCount() == multiSev.getCount(),
          "DetectionEngine runMultiple consolidates heterogeneous channels and survives signal destruction");

    bool chronoOrdered = true;
    for (int i = 1; i < multiChrono.getCount(); ++i) {
        if (multiChrono[i] < multiChrono[i - 1]) {
            chronoOrdered = false;
        }
    }
    check(chronoOrdered, "DetectionEngine runMultiple(false) returns chronologically ordered registry");

    bool sevOrdered = true;
    for (int i = 1; i < multiSev.getCount(); ++i) {
        if (multiSev[i].getSeverity() > multiSev[i - 1].getSeverity()) {
            sevOrdered = false;
        }
    }
    check(sevOrdered, "DetectionEngine runMultiple(true) returns descending normalized severity order");

    std::ostringstream oss;
    oss << assignedEngine << "\n" << multiSev;
    check(oss.str().find("DetectionEngine (detectors=3)") != std::string::npos &&
          oss.str().find("ECG_A") != std::string::npos &&
          oss.str().find("ACC_Z") != std::string::npos,
          "Stream output works cleanly after source ChannelSignal objects are destroyed");

    assignedEngine.clearDetectors();
    check(assignedEngine.isEmpty() && assignedEngine.getDetectorCount() == 0,
          "DetectionEngine clearDetectors empties engine safely");
}

}  // namespace

int main() {
    testEventInvariantsAndOperators();
    testEventRegistryRuleOfThreeAndQueries();
    testPeakDetector();
    testThresholdDetector();
    testWindowAnomalyDetector();
    testDetectionEngineAndLifetimeIndependence();

    std::cout << "Phase 3 Full Integration Tests -> Passed: " << gPassed
              << " | Failed: " << gFailed << "\n";
    return (gFailed == 0) ? 0 : 1;
}