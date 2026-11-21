#include <cassert>
#include <cmath>
#include <iostream>

#include "MetadataSensor.h"
#include "DinamicArr.h"
#include "ChannelStadistics.h"
#include "ChannelSignal.h"
#include "SlicingWindow.h"

namespace {
bool approxEqual(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps;
}
}  // namespace

void testMetadataSensor() {
    MetadataSensor meta("A", "uV", 173.61);
    assert(meta.getCodeOfChannel() == "A");
    assert(meta.getUnitOfMeasure() == "uV");
    assert(approxEqual(meta.getSamplingRate(), 173.61));

    // Valid mutation
    assert(meta.setCodeOfChannel("C2"));
    assert(meta.getCodeOfChannel() == "C2");
    assert(meta.setSamplingRate(250.0));
    assert(approxEqual(meta.getSamplingRate(), 250.0));

    // Invalid mutations must be rejected without corrupting state
    assert(!meta.setCodeOfChannel(""));
    assert(!meta.setCodeOfChannel("A B"));
    assert(!meta.setCodeOfChannel("   "));
    assert(meta.getCodeOfChannel() == "C2");

    assert(!meta.setUnitOfMeasure("m V"));
    assert(meta.getUnitOfMeasure() == "uV");

    assert(!meta.setSamplingRate(0.0));
    assert(!meta.setSamplingRate(-50.0));
    assert(approxEqual(meta.getSamplingRate(), 250.0));

    // Constructor with invalid inputs must fall back to valid invariants
    MetadataSensor badMeta("bad code", "", -10.0);
    assert(badMeta.getCodeOfChannel() == "UNKNOWN");
    assert(badMeta.getUnitOfMeasure() == "uV");
    assert(badMeta.getSamplingRate() > 0.0);

    // Time/index conversions
    assert(meta.ConvertIndexSecs(2.0) == 500);
    assert(meta.ConvertIndexSecs(-1.0) == 0);
    assert(approxEqual(meta.ConvertIndexSecs(500), 2.0));
    assert(approxEqual(meta.ConvertIndexSecs(-5), 0.0));

    std::cout << "[OK] Test 1: MetadataSensor invariantes y conversiones\n";
}

void testDinamicArr() {
    // Empty and zero-initialized constructors
    DinamicArr emptyArr;
    assert(emptyArr.getNumberOfElements() == 0);
    assert(emptyArr.getData() == nullptr);
    assert(std::isnan(emptyArr.getElement(0)));

    DinamicArr zeroArr(4);
    assert(zeroArr.getNumberOfElements() == 4);
    for (int i = 0; i < 4; ++i) {
        assert(approxEqual(zeroArr.getElement(i), 0.0));
    }

    // Negative size constructor safely creates empty array
    DinamicArr invalidSize(-3);
    assert(invalidSize.getNumberOfElements() == 0);
    assert(invalidSize.getData() == nullptr);

    // Block copy constructor & deep copy independence
    const double raw[5] = {10.0, -2.0, 5.0, 8.0, -4.0};
    DinamicArr a(raw, 5);
    assert(a.getNumberOfElements() == 5);
    assert(approxEqual(a.getElement(0), 10.0));
    assert(approxEqual(a.getElement(4), -4.0));

    // Bounds checks (including negative index)
    assert(!a.setElement(99.0, -1));
    assert(!a.setElement(99.0, 5));
    assert(std::isnan(a.getElement(-1)));
    assert(std::isnan(a.getElement(5)));

    // Copy constructor
    DinamicArr b(a);
    assert(b.setElement(999.0, 0));
    assert(approxEqual(a.getElement(0), 10.0));
    assert(approxEqual(b.getElement(0), 999.0));

    // Assignment operator & self-assignment
    DinamicArr c;
    c = a;
    assert(c.getNumberOfElements() == 5);
    assert(approxEqual(c.getElement(2), 5.0));

    DinamicArr* alias = &c;
    c = *alias;  // Self-assignment must not corrupt heap memory
    assert(c.getNumberOfElements() == 5);
    assert(approxEqual(c.getElement(2), 5.0));

    // Assigning empty array to non-empty array must release old memory cleanly
    c = emptyArr;
    assert(c.getNumberOfElements() == 0);
    assert(c.getData() == nullptr);

    std::cout << "[OK] Test 2: DinamicArr Regla de los Tres, limites y auto-asignacion\n";
}

void testChannelStadistics() {
    // Dataset: {2, 4, 4, 4, 5, 5, 7, 9}
    // Mean = 40/8 = 5.0
    // Sum of squared deviations = 9+1+1+1+0+0+4+16 = 32.0
    // Bessel variance = 32 / 7 = 4.5714285714
    // StdDev = sqrt(32/7) = 2.1380899353
    // Energy = 4+16+16+16+25+25+49+81 = 232.0
    const double data[8] = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    ChannelStadistics stats(data, 8);

    assert(approxEqual(stats.getAverage(), 5.0));
    assert(approxEqual(stats.getVariation(), 32.0 / 7.0));
    assert(approxEqual(stats.getStandardDesviation(), std::sqrt(32.0 / 7.0)));
    assert(approxEqual(stats.getMinimum(), 2.0));
    assert(approxEqual(stats.getMaximum(), 9.0));
    assert(approxEqual(stats.getTotalEnergy(), 232.0));

    // Edge case N == 1 (no division by zero)
    const double single[1] = {-7.5};
    assert(stats.updateStadistics(single, 1));
    assert(approxEqual(stats.getAverage(), -7.5));
    assert(approxEqual(stats.getVariation(), 0.0));
    assert(approxEqual(stats.getStandardDesviation(), 0.0));
    assert(approxEqual(stats.getMinimum(), -7.5));
    assert(approxEqual(stats.getMaximum(), -7.5));
    assert(approxEqual(stats.getTotalEnergy(), 56.25));

    // Invalid inputs must fail safely without corrupting initialized state
    ChannelStadistics badStats(nullptr, -5);
    assert(approxEqual(badStats.getAverage(), 0.0));
    assert(approxEqual(badStats.getVariation(), 0.0));
    assert(!stats.updateStadistics(nullptr, 5));
    assert(!stats.updateStadistics(data, 0));
    assert(!stats.updateStadistics(data, -2));

    std::cout << "[OK] Test 3: ChannelStadistics matematica exacta (Bessel) y casos limite\n";
}

void testChannelSignal() {
    const double signalData[6] = {-5.0, 1.0, 3.0, 10.0, -2.0, 5.0};
    ChannelSignal ch1("EEG_F3", "uV", 100.0, signalData, 6);

    assert(ch1.getNumberOfSamples() == 6);
    assert(approxEqual(ch1.getSamplingRate(), 100.0));

    // Verify subRangeMinimum and subRangeMaximum are NOT swapped
    assert(approxEqual(ch1.subRangeMinimum(0, 6, "raw"), -5.0));
    assert(approxEqual(ch1.subRangeMaximum(0, 6, "raw"), 10.0));
    assert(approxEqual(ch1.subRangeAverage(0, 6, "raw"), 2.0));
    assert(approxEqual(ch1.subRangeEnergy(0, 6, "raw"), 25.0 + 1.0 + 9.0 + 100.0 + 4.0 + 25.0));

    // Out-of-bounds sub-ranges must return NaN
    assert(std::isnan(ch1.subRangeAverage(-1, 3, "raw")));
    assert(std::isnan(ch1.subRangeAverage(4, 3, "raw")));
    assert(std::isnan(ch1.subRangeAverage(0, 0, "raw")));
    assert(std::isnan(ch1.subRangeAverage(0, 2, "invalid")));

    // Deep copy via Rule of Zero & independence
    ChannelSignal ch2(ch1);
    assert(ch1.setProcessedElement(100.0, 0));
    assert(approxEqual(ch1.getRawElement(0), -5.0));        // raw stays immutable
    assert(approxEqual(ch1.getProcessedElement(0), 100.0)); // processed changed
    assert(approxEqual(ch2.getProcessedElement(0), -5.0));  // ch2 unaffected

    // Lazy stats recalculation through const pointer
    const ChannelSignal* readOnlyPtr = &ch1;
    assert(approxEqual(readOnlyPtr->getProcessedStatistics().getMaximum(), 100.0));

    // Restore processed samples back to raw
    ch1.restoreProcessedSamples();
    assert(approxEqual(ch1.getProcessedElement(0), -5.0));
    assert(approxEqual(readOnlyPtr->getProcessedStatistics().getMaximum(), 10.0));

    // Self-assignment on ChannelSignal
    ChannelSignal* alias = &ch2;
    ch2 = *alias;
    assert(ch2.getNumberOfSamples() == 6);
    assert(approxEqual(ch2.getRawElement(3), 10.0));

    // Constructor invariant protection when N < 2 or nullptr
    ChannelSignal fallbackCh("Z", "uV", 100.0, nullptr, 1);
    assert(fallbackCh.getNumberOfSamples() == 2);
    assert(approxEqual(fallbackCh.getRawElement(0), 0.0));

    std::cout << "[OK] Test 4: ChannelSignal Regla de Cero, inmutabilidad raw y cache mutable\n";
}

void testSlicingWindow() {
    const double data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    ChannelSignal ch("MIT_MLII", "mV", 10.0, data, 10);

    // Case A: W = 3, stride = 1 -> must visit starts 0..7 (8 windows) including last sample (index 9)
    SlicingWindow win1(&ch, 3, 1);
    assert(approxEqual(win1.calcWindowInSecs(), 0.3));
    int count1 = 0;
    double lastMax = 0.0;
    while (true) {
        lastMax = win1.maxWindow("processed");
        count1++;
        if (win1.itsInTheBorder()) {
            break;
        }
        assert(win1.strideForward());
    }
    assert(count1 == 8);
    assert(approxEqual(lastMax, 10.0));
    assert(approxEqual(win1.calcStartTimeInSecs(), 0.7));

    // Case B: W = 3, stride = 2 on N = 10 -> starts 0, 2, 4, 6 (must terminate without infinite loop!)
    SlicingWindow win2(&ch, 3, 2);
    int count2 = 0;
    do {
        count2++;
        assert(count2 <= 10);
    } while (win2.strideForward());
    assert(count2 == 4);
    assert(win2.getInitIndex() == 6);
    assert(win2.itsInTheBorder());

    // Bounds & nullptr safety
    assert(!win2.setInitIndex(-1));
    assert(!win2.setInitIndex(8));
    assert(win2.setInitIndex(7));
    assert(approxEqual(win2.minWindow("raw"), 8.0));
    assert(approxEqual(win2.maxWindow("raw"), 10.0));

    SlicingWindow oversized(&ch, 999, 0);
    assert(oversized.getWindowLenght() == 10);
    assert(oversized.getStride() == 1);

    SlicingWindow nullWin(nullptr, 3, 1);
    assert(nullWin.itsInTheBorder());
    assert(!nullWin.strideForward());
    assert(std::isnan(nullWin.averageWindow("raw")));

    std::cout << "[OK] Test 5: SlicingWindow zero-copy, fronteras con stride > 1 y seguridad nullptr\n";
}

int main() {
    std::cout << "=== INICIANDO SUITE DE PRUEBAS FASE 1 ===\n";
    testMetadataSensor();
    testDinamicArr();
    testChannelStadistics();
    testChannelSignal();
    testSlicingWindow();
    std::cout << "=== TODAS LAS PRUEBAS DE FASE 1 PASARON EXITOSAMENTE ===\n";
    return 0;
}
