#include "ChannelSignal.h"
#include "DifferentialFilter.h"
#include "MedianFilter.h"
#include "MovingAverageFilter.h"
#include "PointTransformer.h"
#include "ProcessingModule.h"
#include "ProcessingPipeline.h"
#include "SlicingWindow.h"
#include "ThresholdLimiter.h"
#include "WaveRectifier.h"
#include "WindowFilter.h"

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

// Guide §6.1: Multilevel invariant defense + numerical edge cases
void testMultilevelInvariants() {
    const double kNan = std::numeric_limits<double>::quiet_NaN();
    const double kInf = std::numeric_limits<double>::infinity();

    // WindowFilter odd size >= 3 and upper bound clamp
    MovingAverageFilter maEven(4);
    MedianFilter medNeg(-2);
    MovingAverageFilter maHuge(9999999);
    check(maEven.getWindowSize() == 5, "WindowFilter promotes even W=4 to odd W=5");
    check(medNeg.getWindowSize() == 3, "WindowFilter clamps negative W=-2 to 3");
    check(maHuge.getWindowSize() == 100001, "WindowFilter clamps excessive W to safe upper bound");

    // PointTransformer scaleFactor != 0.0 and finite
    ThresholdLimiter limZeroScale(-10.0, 10.0, 0.0);
    check(approxEqual(limZeroScale.getScaleFactor(), 1.0),
          "PointTransformer constructor falls back to 1.0 when scaleFactor == 0.0");
    check(limZeroScale.setScaleFactor(-2.5) && approxEqual(limZeroScale.getScaleFactor(), -2.5),
          "ThresholdLimiter accepts negative finite scaleFactor");
    check(!limZeroScale.setScaleFactor(0.0) && approxEqual(limZeroScale.getScaleFactor(), -2.5),
          "PointTransformer::setScaleFactor(0.0) rejects and preserves prior value");
    check(!limZeroScale.setScaleFactor(kNan) && approxEqual(limZeroScale.getScaleFactor(), -2.5),
          "PointTransformer::setScaleFactor(NaN) rejects and preserves prior value");
    check(!limZeroScale.setScaleFactor(kInf) && approxEqual(limZeroScale.getScaleFactor(), -2.5),
          "PointTransformer::setScaleFactor(+Inf) rejects and preserves prior value");

    // ThresholdLimiter L_min < L_max and finite
    ThresholdLimiter limBad(5.0, 5.0);
    check(approxEqual(limBad.getLowerLimit(), -1.0) && approxEqual(limBad.getUpperLimit(), 1.0),
          "ThresholdLimiter constructor rejects L_min >= L_max with fallback [-1, 1]");
    check(!limBad.setLimits(4.0, 2.0) && approxEqual(limBad.getLowerLimit(), -1.0),
          "ThresholdLimiter::setLimits rejects L_min > L_max and preserves prior bounds");
    check(!limBad.setLimits(-kInf, kInf) && approxEqual(limBad.getUpperLimit(), 1.0),
          "ThresholdLimiter::setLimits rejects +/-Inf bounds");

    // WaveRectifier strict non-negativity invariant (scaleFactor > 0.0 even via base reference)
    WaveRectifier rectBadGain(RectifierMode::FullWave, -3.0);
    check(approxEqual(rectBadGain.getScaleFactor(), 1.0),
          "WaveRectifier constructor rejects negative scaleFactor with fallback 1.0");
    PointTransformer& ptRef = rectBadGain;
    check(!ptRef.setScaleFactor(-2.0) && approxEqual(rectBadGain.getScaleFactor(), 1.0),
          "WaveRectifier virtual setScaleFactor rejects negative gain via PointTransformer&");
    check(ptRef.setScaleFactor(2.0) && approxEqual(rectBadGain.processSample(-4.0), 8.0),
          "WaveRectifier accepts positive gain and outputs non-negative values");
    rectBadGain.setMode(RectifierMode::HalfWave);
    check(approxEqual(rectBadGain.processSample(-4.0), 0.0),
          "WaveRectifier HalfWave zeroes negative samples");

    // WindowFilter NaN resilience (does not poison circular buffer)
    MovingAverageFilter ma3(3);
    check(approxEqual(ma3.processSample(10.0), 10.0), "MovingAverage warm-up n=1");
    check(approxEqual(ma3.processSample(20.0), 15.0), "MovingAverage warm-up n=2");
    check(approxEqual(ma3.processSample(30.0), 20.0), "MovingAverage full window n=3");
    check(std::isnan(ma3.processSample(kNan)), "WindowFilter passes NaN through safely");
    check(approxEqual(ma3.processSample(40.0), 30.0), "WindowFilter buffer remains unpoisoned after NaN");

    // MedianFilter true even-n warm-up + heap fallback (W > 64)
    MedianFilter med5(5);
    check(approxEqual(med5.processSample(10.0), 10.0), "MedianFilter warm-up n=1");
    check(approxEqual(med5.processSample(20.0), 15.0), "MedianFilter even warm-up n=2 -> (10+20)/2");
    check(approxEqual(med5.processSample(715897.0), 20.0), "MedianFilter suppresses impulse spike at n=3");
    check(approxEqual(med5.processSample(12.0), 16.0), "MedianFilter even warm-up n=4 -> (12+20)/2");
    check(approxEqual(med5.processSample(14.0), 14.0), "MedianFilter full W=5 eliminates spike");

    MedianFilter med67(67);
    double lastMed = 0.0;
    for (int i = 1; i <= 67; ++i) {
        lastMed = med67.processSample(static_cast<double>(i));
    }
    check(approxEqual(lastMed, 34.0), "MedianFilter heap fallback (W=67) computes exact median");

    // DifferentialFilter TKEO dimensional consistency (W=3 fixed)
    DifferentialFilter tkeo;
    check(tkeo.getWindowSize() == 3, "DifferentialFilter fixes W=3");
    check(approxEqual(tkeo.processSample(2.0), 0.0), "DifferentialFilter n=1 -> 0.0");
    check(approxEqual(tkeo.processSample(5.0), 0.0), "DifferentialFilter n=2 -> 0.0");
    check(approxEqual(tkeo.processSample(3.0), 5.0 * 5.0 - 2.0 * 3.0),
          "DifferentialFilter n=3 -> exact TKEO x[n-1]^2 - x[n-2]*x[n]");

    // Lossless serialization includes active state and scaleFactor
    rectBadGain.setActive(false);
    const std::string rectSer = rectBadGain.serialize();
    check(rectSer.find("HALF_WAVE") != std::string::npos &&
          rectSer.find("scale=2.000000") != std::string::npos &&
          rectSer.find("active=0") != std::string::npos,
          "WaveRectifier serialize preserves mode, scaleFactor, and active state");
}

// Guide §6.2: Scientific impulse-noise benchmark on ChannelSignal (50 samples = 10.0, spike 950.0 at idx 25)
void testScientificImpulseNoiseBenchmark() {
    double data[50];
    for (int i = 0; i < 50; ++i) {
        data[i] = 10.0;
    }
    data[25] = 950.0;

    ChannelSignal signal("EEG_BENCH", "uV", 128.0, data, 50);

    // 1) MovingAverageFilter (W=5) smears spike across 5 consecutive samples [25..29]
    MovingAverageFilter ma5(5);
    check(signal.applyModule(ma5), "ChannelSignal::applyModule(MovingAverageFilter) succeeds");

    int contaminatedCount = 0;
    for (int i = 0; i < 50; ++i) {
        if (!approxEqual(signal.getProcessedElement(i), 10.0)) {
            ++contaminatedCount;
        }
    }
    check(contaminatedCount == 5,
          "MovingAverageFilter(W=5) smears impulse spike across exactly 5 consecutive samples");
    check(signal.getProcessedStatistics().getVariation() > 0.0 &&
          signal.getProcessedStatistics().getAverage() > 10.0,
          "MovingAverageFilter leaves residual variance and shifted mean in ChannelStadistics");

    // Raw buffer must remain completely untouched
    check(approxEqual(signal.getRawElement(25), 950.0),
          "ChannelSignal::applyModule preserves immutable rawSamples");

    // 2) Restore original signal and apply MedianFilter (W=5)
    signal.restoreProcessedSamples();
    check(approxEqual(signal.getProcessedElement(25), 950.0),
          "restoreProcessedSamples recovers original spike before median pass");

    MedianFilter med5(5);
    check(signal.applyModule(med5), "ChannelSignal::applyModule(MedianFilter) succeeds");

    bool allExactTen = true;
    for (int i = 0; i < 50; ++i) {
        if (!approxEqual(signal.getProcessedElement(i), 10.0)) {
            allExactTen = false;
        }
    }
    check(allExactTen, "MedianFilter(W=5) removes 100% of impulse spike");
    check(approxEqual(signal.getProcessedStatistics().getAverage(), 10.0) &&
          approxEqual(signal.getProcessedStatistics().getVariation(), 0.0),
          "ChannelStadistics reports exact mean=10.0 and variance=0.0 after MedianFilter");

    // Invalid sourceBuffer returns false without mutating processedSamples
    check(!signal.applyModule(ma5, "invalid_buf") &&
          approxEqual(signal.getProcessedStatistics().getVariation(), 0.0),
          "ChannelSignal::applyModule rejects invalid buffer selector without mutating signal");
}

// Guide §6.3 & §6.4: 5-stage polymorphic cascade (resize > 4), bypass, Rule of Three, SlicingWindow, Valgrind cleanup
void testCascadeRuleOfThreeAndSlicingWindow() {
    expectInvalidArg([]() { ProcessingPipeline bad(-1); },
                     "ProcessingPipeline rejects negative initialCapacity");

    ProcessingPipeline pipe(4);
    check(pipe.getCapacity() == 4 && pipe.isEmpty(), "ProcessingPipeline starts empty with capacity 4");

    // Chain all 5 concrete subclasses -> triggers manual resize (4 -> 8)
    pipe.addStage(MedianFilter(3));
    pipe.addStage(MovingAverageFilter(3));
    pipe.addStage(DifferentialFilter());
    pipe.addStage(WaveRectifier(RectifierMode::FullWave, 1.0));
    ProcessingModule* adoptedLimiter = new ThresholdLimiter(0.0, 50.0, 1.0);
    pipe.adoptStage(adoptedLimiter);

    check(pipe.getCount() == 5 && pipe.getCapacity() == 8,
          "ProcessingPipeline resizes manually when exceeding initial capacity 4");

    expectInvalidArg([&pipe]() { pipe.adoptStage(nullptr); },
                     "ProcessingPipeline::adoptStage rejects nullptr");
    expectInvalidArg([&pipe, adoptedLimiter]() { pipe.adoptStage(adoptedLimiter); },
                     "ProcessingPipeline::adoptStage rejects duplicate owned pointer safely");
    expectOutOfRange([&pipe]() { (void)pipe[5]; },
                     "ProcessingPipeline::operator[] bounds-checks");

    // Deep copy constructor, copy assignment, and self-assignment
    ProcessingPipeline copyPipe(pipe);
    check(copyPipe == pipe && !(copyPipe != pipe), "Copy constructor produces equal pipeline");
    check(&copyPipe[0] != &pipe[0] && &copyPipe[4] != &pipe[4],
          "Copy constructor deep-clones stages into independent heap objects");

    ProcessingPipeline assignedPipe(0);
    assignedPipe = pipe;
    ProcessingPipeline& selfAlias = assignedPipe;
    assignedPipe = selfAlias;
    check(assignedPipe == pipe && assignedPipe.getCount() == 5,
          "Copy assignment and self-assignment preserve pipeline integrity");

    // Apply 5-stage cascade on a synthetic oscillating signal with a spike
    const double rawInput[12] = {
        0.0, 2.0, 5.0, 200.0, 4.0, -3.0, -6.0, -2.0, 1.0, 4.0, 2.0, 0.0
    };
    ChannelSignal sig("ECG_CASCADE", "mV", 100.0, rawInput, 12);

    check(sig.applyPipeline(pipe, "raw"), "ChannelSignal::applyPipeline(raw) succeeds");
    check(approxEqual(sig.getRawElement(3), 200.0),
          "Raw buffer remains intact after 5-stage pipeline");

    // Verify ThresholdLimiter [0, 50] capped every processed sample
    bool withinLimiterBounds = true;
    for (int i = 0; i < 12; ++i) {
        const double v = sig.getProcessedElement(i);
        if (v < -1e-9 || v > 50.0 + 1e-9) {
            withinLimiterBounds = false;
        }
    }
    check(withinLimiterBounds, "5-stage cascade enforces final ThresholdLimiter bounds [0, 50]");

    // SlicingWindow reads processed vs raw cleanly without copying
    SlicingWindow win(&sig, 4, 2);
    check(approxEqual(win.maxWindow("raw"), 200.0), "SlicingWindow reads unmodified raw peak (200.0)");
    check(win.maxWindow("processed") <= 50.0 + 1e-9,
          "SlicingWindow reads filtered processed window");

    // Bypassing an intermediate stage (index 4: ThresholdLimiter or index 0: MedianFilter) alters output
    const double maxWithMedian = sig.getProcessedStatistics().getMaximum();
    check(copyPipe.setStageActive(0, false) && !copyPipe[0].isActive(),
          "setStageActive deactivates stage 0 on copyPipe");
    check(pipe[0].isActive(),
          "Modifying copyPipe does not affect original pipe (true deep copy)");

    check(sig.applyPipeline(copyPipe, "raw"), "applyPipeline with bypassed MedianFilter succeeds");
    // Without MedianFilter, the 200.0 spike enters MovingAverage/TKEO and saturates limiter at 50.0
    check(sig.getProcessedStatistics().getMaximum() >= maxWithMedian &&
          approxEqual(sig.getProcessedStatistics().getMaximum(), 50.0),
          "Bypassing MedianFilter measurably alters cascade output");

    // Cross-channel reuse must reset filter history automatically
    const double cleanFlat[5] = {7.0, 7.0, 7.0, 7.0, 7.0};
    ChannelSignal flatSig("CH_CLEAN", "mV", 50.0, cleanFlat, 5);
    ProcessingPipeline simplePipe(2);
    simplePipe.addStage(MovingAverageFilter(3));
    check(sig.applyPipeline(simplePipe, "raw"), "Run simplePipe on spiky channel first");
    check(flatSig.applyPipeline(simplePipe, "raw"), "Reuse simplePipe on clean flat channel");
    check(approxEqual(flatSig.getProcessedElement(0), 7.0) &&
          approxEqual(flatSig.getProcessedStatistics().getVariation(), 0.0),
          "ChannelSignal::applyPipeline resets filter history between channels");

    // Invalid buffer selector, removeStage, stream serialization, and clear
    check(!flatSig.applyPipeline(simplePipe, "wrong"),
          "ChannelSignal::applyPipeline rejects invalid buffer selector");
    check(assignedPipe.removeStage(2) && assignedPipe.getCount() == 4,
          "removeStage deletes stage and shifts remaining stages");
    check(!assignedPipe.removeStage(99), "removeStage rejects out-of-range index");

    std::ostringstream oss;
    oss << assignedPipe;
    check(oss.str().find("ProcessingPipeline (stages=4)") != std::string::npos,
          "ProcessingPipeline operator<< prints formatted summary");

    assignedPipe.clear();
    check(assignedPipe.isEmpty() && assignedPipe.getCount() == 0,
          "ProcessingPipeline::clear empties stages cleanly");
}

}  // namespace

int main() {
    testMultilevelInvariants();
    testScientificImpulseNoiseBenchmark();
    testCascadeRuleOfThreeAndSlicingWindow();

    std::cout << "Phase 2 Full Integration & Guide §6 Tests -> Passed: " << gPassed
              << " | Failed: " << gFailed << "\n";
    return (gFailed == 0) ? 0 : 1;
}
