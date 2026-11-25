#include "DetectecEvent.h"
#include "EventRegistry.h"
#include "PeakEvent.h"
#include "StatisticalAnomalyEvent.h"
#include "ThresholdEvent.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

int gPassed = 0;
int gFailed = 0;

bool approxEqual(double a, double b, double tol = 1e-9) {
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
        std::cerr << "[FAIL] Wrong exception type (expected invalid_argument): " << label << "\n";
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
        std::cerr << "[FAIL] Wrong exception type (expected out_of_range): " << label << "\n";
    }
}

void testInvariants() {
    const double kNan = std::numeric_limits<double>::quiet_NaN();
    const double kInf = std::numeric_limits<double>::infinity();

    // Base DetectedEvent invariants (tested via PeakEvent and ThresholdEvent)
    expectInvalidArg([]() {
        PeakEvent("", "mV", 1, 0.1, 1.0, 2.0, 1.0, true);
    }, "Empty channel code rejected");

    expectInvalidArg([]() {
        PeakEvent("ECG1", "", 1, 0.1, 1.0, 2.0, 1.0, true);
    }, "Empty unit rejected");

    expectInvalidArg([]() {
        PeakEvent("ECG1", "mV", -1, 0.1, 1.0, 2.0, 1.0, true);
    }, "Negative index rejected");

    expectInvalidArg([]() {
        ThresholdEvent("ECG1", "mV", 10, 9, 0.5, 0.6, 1.0, 1.0, 2.0, ThresholdEvent::CrossingType::UPPER);
    }, "endIndex < initIndex rejected");

    expectInvalidArg([]() {
        ThresholdEvent("ECG1", "mV", 5, 8, -0.1, 0.6, 1.0, 1.0, 2.0, ThresholdEvent::CrossingType::UPPER);
    }, "Negative startTime rejected");

    expectInvalidArg([]() {
        ThresholdEvent("ECG1", "mV", 5, 8, 1.0, 0.9, 1.0, 1.0, 2.0, ThresholdEvent::CrossingType::UPPER);
    }, "endTime < startTime rejected");

    expectInvalidArg([kNan]() {
        PeakEvent("ECG1", "mV", 1, kNan, 1.0, 2.0, 1.0, true);
    }, "NaN timestamp rejected");

    expectInvalidArg([kInf]() {
        PeakEvent("ECG1", "mV", 1, kInf, 1.0, 2.0, 1.0, true);
    }, "+Inf timestamp rejected");

    expectInvalidArg([]() {
        PeakEvent("ECG1", "mV", 1, 0.1, -0.01, 2.0, 1.0, true);
    }, "Negative severity rejected");

    expectInvalidArg([kInf]() {
        PeakEvent("ECG1", "mV", 1, 0.1, kInf, 2.0, 1.0, true);
    }, "+Inf severity rejected");

    // PeakEvent specific invariants
    expectInvalidArg([kNan]() {
        PeakEvent("ECG1", "mV", 1, 0.1, 1.0, kNan, 1.0, true);
    }, "PeakEvent NaN amplitude rejected");

    expectInvalidArg([]() {
        PeakEvent("ECG1", "mV", 1, 0.1, 1.0, 2.0, -0.5, true);
    }, "PeakEvent negative prominence rejected");

    // Valid negative amplitude with positive polarity (local maximum below 0)
    PeakEvent negMax("EEG1", "uV", 4, 0.04, 2.5, -5.0, 3.0, true);
    check(approxEqual(negMax.getPeakAmplitude(), -5.0) && negMax.isPositivePeak(),
          "PeakEvent allows negative amplitude on positive local maximum");

    // ThresholdEvent specific invariants
    expectInvalidArg([]() {
        ThresholdEvent("ECG1", "mV", 2, 5, 0.2, 0.5, 1.5, 3.0, 2.9, ThresholdEvent::CrossingType::UPPER);
    }, "ThresholdEvent UPPER with extreme < threshold rejected");

    expectInvalidArg([]() {
        ThresholdEvent("ECG1", "mV", 2, 5, 0.2, 0.5, 1.5, -3.0, -2.9, ThresholdEvent::CrossingType::LOWER);
    }, "ThresholdEvent LOWER with extreme > threshold rejected");

    expectInvalidArg([kNan]() {
        ThresholdEvent("ECG1", "mV", 2, 5, 0.2, 0.5, 1.5, kNan, 4.0, ThresholdEvent::CrossingType::UPPER);
    }, "ThresholdEvent NaN threshold rejected");

    // Exact boundary equality is valid
    ThresholdEvent exactBound("ECG1", "mV", 2, 2, 0.2, 0.2, 0.0, 3.0, 3.0, ThresholdEvent::CrossingType::UPPER);
    check(exactBound.getDurationInSamples() == 1 && approxEqual(exactBound.getDurationInSecs(), 0.0),
          "ThresholdEvent allows exact boundary extreme == threshold");

    // StatisticalAnomalyEvent specific invariants
    expectInvalidArg([kInf]() {
        StatisticalAnomalyEvent("ECG1", "mV", 0, 9, 0.0, 0.9, 3.0, kInf, 2.0, 0.0, 1.0, 1);
    }, "StatisticalAnomalyEvent +Inf Z-score rejected");

    expectInvalidArg([]() {
        StatisticalAnomalyEvent("ECG1", "mV", 0, 9, 0.0, 0.9, 3.0, 3.5, -0.1, 0.0, 1.0, 1);
    }, "StatisticalAnomalyEvent negative energyRatio rejected");

    expectInvalidArg([]() {
        StatisticalAnomalyEvent("ECG1", "mV", 0, 9, 0.0, 0.9, 3.0, 3.5, 2.0, 0.0, -1.0, 1);
    }, "StatisticalAnomalyEvent negative baselineStdDev rejected");

    expectInvalidArg([]() {
        StatisticalAnomalyEvent("ECG1", "mV", 0, 9, 0.0, 0.9, 3.0, 3.5, 2.0, 0.0, 1.0, 0);
    }, "StatisticalAnomalyEvent involvedWindows < 1 rejected");
}

void testPolymorphismAndStreamHygiene() {
    PeakEvent peak("ECG1", "mV", 12, 0.12, 4.2, 1.85, 1.40, true);
    ThresholdEvent thr("ECG1", "mV", 20, 24, 0.20, 0.24, 3.1, 1.0, 1.9, ThresholdEvent::CrossingType::UPPER);
    StatisticalAnomalyEvent anom("ECG2", "mV", 30, 49, 0.30, 0.49, 5.0, -5.0, 9.0, 0.1, 0.2, 3);

    check(peak.getType() == "PEAK" && approxEqual(peak.getCharacteristicValue(), 1.85),
          "PeakEvent type and characteristic value");
    check(peak.getDurationInSamples() == 1 && approxEqual(peak.getDurationInSecs(), 0.0),
          "PeakEvent degenerate interval duration");

    check(thr.getType() == "THRESHOLD" && approxEqual(thr.getCharacteristicValue(), 1.9),
          "ThresholdEvent type and characteristic value");
    check(thr.getDurationInSamples() == 5 && approxEqual(thr.getDurationInSecs(), 0.04),
          "ThresholdEvent sample and time duration");

    check(anom.getType() == "STATISTICAL_ANOMALY" && approxEqual(anom.getCharacteristicValue(), -5.0),
          "StatisticalAnomalyEvent type and characteristic value");
    check(anom.getInvolvedWindows() == 3 && approxEqual(anom.getEnergyRatio(), 9.0),
          "StatisticalAnomalyEvent specific getters");

    // Polymorphic clone creates distinct address with equal content
    const DetectedEvent& baseRef = anom;
    DetectedEvent* cloned = baseRef.clone();
    check(cloned != &anom && *cloned == anom, "Polymorphic clone deep equality and distinct address");
    delete cloned;

    // Stream formatting flags and precision must remain untouched after operator<<
    std::ostringstream oss;
    oss.precision(6);
    const std::ios::fmtflags flagsBefore = oss.flags();
    const std::streamsize precBefore = oss.precision();

    oss << peak << "\n" << thr << "\n" << anom;

    check(oss.flags() == flagsBefore && oss.precision() == precBefore,
          "DetectedEvent::print does not pollute caller stream state");
    check(oss.str().find("[PEAK]") != std::string::npos &&
          oss.str().find("[THRESHOLD]") != std::string::npos &&
          oss.str().find("[STATISTICAL_ANOMALY]") != std::string::npos,
          "Polymorphic operator<< emits all event tags");
}

void testComparisonAndEquality() {
    PeakEvent a("CH1", "mV", 10, 1.0, 3.0, 5.0, 2.0, true);
    PeakEvent aClose("CH1", "mV", 10, 1.0, 3.0 + 1e-11, 5.0 + 1e-11, 2.0, true);
    PeakEvent aDiffSeverity("CH1", "mV", 10, 1.0, 4.5, 5.0, 2.0, true);
    PeakEvent aDiffIndex("CH1", "mV", 11, 1.1, 3.0, 5.0, 2.0, true);
    PeakEvent aOtherChannel("CH2", "mV", 10, 1.0, 3.0, 5.0, 2.0, true);
    ThresholdEvent tSameSpan("CH1", "mV", 10, 10, 1.0, 1.0, 3.0, 4.0, 5.0, ThresholdEvent::CrossingType::UPPER);

    check(a == aClose && !(a != aClose), "operator== accepts differences below 1e-9 tolerance");
    check(a != aDiffIndex, "operator== distinguishes different sample indices");

    // Consistency between operator< and operator== when only severity differs
    check(aDiffSeverity < a, "operator< breaks index tie by descending severity");
    check(a != aDiffSeverity, "operator== distinguishes events with different severity (no a<b && a==b contradiction)");

    // Type tie-break ("PEAK" < "THRESHOLD") and channel tie-break ("CH1" < "CH2")
    check(a < tSameSpan, "operator< breaks severity tie by type lexicographically");
    check(a < aOtherChannel, "operator< breaks type tie by channel code lexicographically");
}

void testRegistryCoreAndErrors() {
    expectInvalidArg([]() {
        EventRegistry bad(-1);
    }, "EventRegistry negative initialCapacity rejected");

    EventRegistry reg(0);
    check(reg.isEmpty() && reg.getCount() == 0 && reg.getCapacity() == 0,
          "EventRegistry(0) starts empty with capacity 0");

    expectOutOfRange([&reg]() {
        (void)reg.get(0);
    }, "get(0) on empty registry throws out_of_range");

    expectInvalidArg([&reg]() {
        reg.adopt(nullptr);
    }, "adopt(nullptr) throws invalid_argument");

    PeakEvent p1("CH1", "mV", 5, 0.5, 2.0, 3.0, 1.5, true);
    reg.add(p1);
    check(reg.getCount() == 1 && reg.getCapacity() >= 1 && !reg.isEmpty(),
          "EventRegistry(0) grows on first add()");

    DetectedEvent* rawPtr = new PeakEvent("CH1", "mV", 8, 0.8, 3.0, 4.0, 2.0, true);
    reg.adopt(rawPtr);
    check(reg.getCount() == 2, "adopt transfers ownership cleanly");

    // Adopting the exact same already-owned pointer must throw WITHOUT double-freeing
    expectInvalidArg([&reg, rawPtr]() {
        reg.adopt(rawPtr);
    }, "adopt rejects duplicate pointer address safely");

    check(&reg[0] == &reg.get(0), "operator[] and get() refer to the same stored event");
    const EventRegistry& constRef = reg;
    check(constRef[1].getInitIndex() == 8, "const operator[] works");

    expectOutOfRange([&reg]() {
        (void)reg[-1];
    }, "operator[](-1) throws out_of_range");

    expectOutOfRange([&reg]() {
        (void)reg[2];
    }, "operator[](count) throws out_of_range");

    const int capBeforeClear = reg.getCapacity();
    reg.clear();
    check(reg.isEmpty() && reg.getCount() == 0 && reg.getCapacity() == capBeforeClear,
          "clear() removes all events while preserving capacity");
}

void testRegistryRuleOfThreeAndOperators() {
    EventRegistry r1(2);
    r1 += PeakEvent("CH1", "mV", 1, 0.1, 2.0, 1.5, 1.0, true);
    r1 += ThresholdEvent("CH1", "mV", 3, 6, 0.3, 0.6, 4.0, 1.0, 2.2, ThresholdEvent::CrossingType::UPPER);
    r1 += StatisticalAnomalyEvent("CH2", "mV", 7, 15, 0.7, 1.5, 5.5, 5.5, 8.0, 0.0, 0.5, 2);

    // Copy constructor deep-copy independence
    EventRegistry rCopy(r1);
    check(rCopy == r1 && !(rCopy != r1), "Copy constructor produces equal registry");
    check(&rCopy[0] != &r1[0], "Copy constructor allocates independent event clones");

    // Copy assignment overwriting populated target
    EventRegistry rAssigned(1);
    rAssigned += PeakEvent("OLD", "V", 99, 9.9, 1.0, 1.0, 1.0, false);
    rAssigned = r1;
    check(rAssigned == r1 && &rAssigned[1] != &r1[1], "Copy assignment deep-copies and replaces prior contents");

    // Self-assignment via alias (avoids -Wself-assign-overloaded while exercising this == &other)
    EventRegistry& selfAlias = rAssigned;
    rAssigned = selfAlias;
    check(rAssigned == r1 && rAssigned.getCount() == 3, "Self-assignment leaves registry intact");

    // Self-concatenation (reg += reg)
    rCopy += rCopy;
    check(rCopy.getCount() == 6 && rCopy[0] == rCopy[3] && rCopy[2] == rCopy[5],
          "Self-concatenation (reg += reg) duplicates safely without invalidating pointers");

    // Non-mutating operator+
    EventRegistry rOther(1);
    rOther += PeakEvent("CH3", "uV", 20, 2.0, 1.2, -3.0, 0.9, false);
    EventRegistry rSum = r1 + rOther;
    check(rSum.getCount() == 4 && r1.getCount() == 3 && rOther.getCount() == 1,
          "operator+ combines without mutating operands");
    check(rSum[3] == rOther[0], "operator+ preserves operand order");

    // Stream insertion of registry
    std::ostringstream oss;
    oss << r1;
    check(oss.str().find("EventRegistry (count=3)") != std::string::npos,
          "EventRegistry operator<< formats header and contents");
}

void testSortingAndFiltering() {
    EventRegistry reg(4);
    reg += StatisticalAnomalyEvent("CH1", "mV", 30, 40, 3.0, 4.0, 2.0, 2.0, 3.0, 0.0, 1.0, 2);
    reg += PeakEvent("CH1", "mV", 5, 0.5, 7.5, 4.0, 3.0, true);
    reg += ThresholdEvent("CH1", "mV", 10, 25, 1.0, 2.5, 4.5, 1.0, 3.5, ThresholdEvent::CrossingType::UPPER);
    reg += PeakEvent("CH2", "mV", 5, 0.5, 1.0, 2.0, 1.0, true);

    reg.sortChronologically();
    check(reg[0].getInitIndex() == 5 && approxEqual(reg[0].getSeverity(), 7.5) &&
          reg[1].getInitIndex() == 5 && approxEqual(reg[1].getSeverity(), 1.0) &&
          reg[2].getInitIndex() == 10 &&
          reg[3].getInitIndex() == 30,
          "sortChronologically orders by index and breaks ties by descending severity");

    reg.sortBySeverity();
    check(approxEqual(reg[0].getSeverity(), 7.5) &&
          approxEqual(reg[1].getSeverity(), 4.5) &&
          approxEqual(reg[2].getSeverity(), 2.0) &&
          approxEqual(reg[3].getSeverity(), 1.0),
          "sortBySeverity orders globally by descending normalized severity");

    check(reg.countByType("PEAK") == 2 &&
          reg.countByType("THRESHOLD") == 1 &&
          reg.countByType("STATISTICAL_ANOMALY") == 1 &&
          reg.countByType("UNKNOWN") == 0,
          "countByType counts accurately");

    EventRegistry peaksOnly = reg.filterByType("PEAK");
    check(peaksOnly.getCount() == 2 && peaksOnly[0].getType() == "PEAK",
          "filterByType returns matching clones");

    // Time range overlap: [2.0, 3.5] overlaps Threshold [1.0, 2.5] and Anomaly [3.0, 4.0], excludes Peaks at 0.5
    EventRegistry windowSlice = reg.filterByTimeRange(2.0, 3.5);
    check(windowSlice.getCount() == 2 &&
          windowSlice.countByType("THRESHOLD") == 1 &&
          windowSlice.countByType("STATISTICAL_ANOMALY") == 1,
          "filterByTimeRange includes partial overlaps and excludes disjoint events");

    expectInvalidArg([&reg]() {
        (void)reg.filterByTimeRange(3.0, 1.0);
    }, "filterByTimeRange rejects startTime > endTime");

    const double kNan = std::numeric_limits<double>::quiet_NaN();
    expectInvalidArg([&reg, kNan]() {
        (void)reg.filterByTimeRange(kNan, 2.0);
    }, "filterByTimeRange rejects NaN bound");
}

}  // namespace

int main() {
    testInvariants();
    testPolymorphismAndStreamHygiene();
    testComparisonAndEquality();
    testRegistryCoreAndErrors();
    testRegistryRuleOfThreeAndOperators();
    testSortingAndFiltering();

    std::cout << "Phase 3 (Blocks 1 & 2) Tests -> Passed: " << gPassed
              << " | Failed: " << gFailed << "\n";
    return (gFailed == 0) ? 0 : 1;
}