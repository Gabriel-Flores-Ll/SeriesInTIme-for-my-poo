#include "ChannelComparator.h"
#include "ChannelSignal.h"
#include "ChronologicalComparator.h"
#include "DetectecEvent.h"
#include "DetectionEngine.h"
#include "DurationComparator.h"
#include "EventComparator.h"
#include "EventRegistry.h"
#include "EventSorter.h"
#include "MedianFilter.h"
#include "PeakDetector.h"
#include "PeakEvent.h"
#include "ProcessingPipeline.h"
#include "SeverityComparator.h"
#include "StatisticalAnomalyEvent.h"
#include "ThresholdDetector.h"
#include "ThresholdEvent.h"
#include "WindowAnomalyDetector.h"

#include <cmath>
#include <iostream>
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

bool samePointerMultiset(const DetectedEvent* const* before,
                         const EventRegistry& after,
                         int n) {
    if (after.getCount() != n) {
        return false;
    }
    bool* matched = new bool[n]();
    bool allMatched = true;
    for (int i = 0; i < n; ++i) {
        const DetectedEvent* addr = &after[i];
        bool found = false;
        for (int j = 0; j < n; ++j) {
            if (!*(matched + j) && *(before + j) == addr) {
                *(matched + j) = true;
                found = true;
                break;
            }
        }
        if (!found) {
            allMatched = false;
            break;
        }
    }
    delete[] matched;
    return allMatched;
}

void testSortingBoundaryConditions() {
    const SeverityComparator bySevDesc(true);
    const ChronologicalComparator byTimeAsc(true);

    // Empty registry
    EventRegistry empty(0);
    EventSorter::quickSort(empty);
    EventSorter::quickSort(empty, bySevDesc);
    EventSorter::insertionSort(empty, byTimeAsc);
    check(empty.isEmpty() && EventSorter::isSorted(empty) && EventSorter::isSorted(empty, bySevDesc),
          "EventSorter handles empty registry safely");

    // Single-element registry
    EventRegistry single(1);
    single += PeakEvent("CH1", "mV", 5, 0.5, 2.0, 4.0, 2.0, true);
    const DetectedEvent* singleAddr = &single[0];
    EventSorter::quickSort(single);
    EventSorter::insertionSort(single, bySevDesc);
    check(single.getCount() == 1 && &single[0] == singleAddr && EventSorter::isSorted(single),
          "EventSorter handles single-element registry without moving memory");

    // 40 reverse-sorted + duplicate-severity elements (stresses 3-way partition and pivot selection)
    EventRegistry rev(40);
    for (int i = 39; i >= 0; --i) {
        rev += PeakEvent("ECG", "mV", i * 2, i * 0.02, static_cast<double>(i % 5), 5.0, 2.0, true);
    }
    check(!EventSorter::isSorted(rev), "Reverse-ordered registry starts unsorted");
    EventSorter::quickSort(rev);
    check(EventSorter::isSorted(rev) && rev[0].getInitIndex() == 0 && rev[39].getInitIndex() == 78,
          "Quicksort relational sorts reverse-ordered 40-element registry accurately");

    // Already sorted stays sorted (no O(n^2) failure)
    EventSorter::quickSort(rev);
    check(EventSorter::isSorted(rev), "Quicksort relational handles already-sorted input");

    // All-identical elements (stresses 3-way Dutch National Flag partitioning)
    EventRegistry allSame(15);
    for (int i = 0; i < 15; ++i) {
        allSame += PeakEvent("EEG", "uV", 10, 0.1, 3.0, 12.0, 4.0, true);
    }
    EventSorter::quickSort(allSame);
    EventSorter::quickSort(allSame, bySevDesc);
    EventSorter::insertionSort(allSame, bySevDesc);
    check(allSame.getCount() == 15 && EventSorter::isSorted(allSame) && EventSorter::isSorted(allSame, bySevDesc),
          "Quicksort and Insertion Sort handle 15 identical events cleanly");
}

void testPolymorphicFunctorsAndAntiSlicing() {
    EventRegistry reg(6);
    reg += PeakEvent("V5", "mV", 50, 0.50, 4.2, 6.5, 3.1, true);
    reg += ThresholdEvent("MLII", "mV", 10, 25, 0.10, 0.25, 7.8, 2.0, 5.9, ThresholdEvent::CrossingType::UPPER);
    reg += StatisticalAnomalyEvent("AF4", "uV", 30, 80, 0.30, 0.80, 5.5, -5.5, 9.0, 1.0, 2.0, 4);
    reg += PeakEvent("AF4", "uV", 12, 0.12, 7.8, -8.4, 6.0, false);
    reg += ThresholdEvent("V5", "mV", 5, 10, 0.05, 0.10, 2.1, -1.0, -3.2, ThresholdEvent::CrossingType::LOWER);
    reg += StatisticalAnomalyEvent("MLII", "mV", 40, 60, 0.40, 0.60, 5.5, 3.2, 30.25, 0.0, 1.5, 2);

    const int n = reg.getCount();
    const DetectedEvent* addressesBefore[6];
    for (int i = 0; i < n; ++i) {
        addressesBefore[i] = &reg[i];
    }

    // 1) Polymorphic dispatch via EventComparator base pointer array
    EventComparator* comparators[4] = {
        new SeverityComparator(true),
        new ChronologicalComparator(true),
        new DurationComparator(true),
        new ChannelComparator(true)
    };

    // Severity descending via Insertion Sort (ties at 7.8 and 5.5 must break chronologically)
    EventSorter::insertionSort(reg, *comparators[0]);
    check(EventSorter::isSorted(reg, *comparators[0]),
          "Insertion Sort with SeverityComparator(DESC) orders correctly");
    check(approxEqual(reg[0].getSeverity(), 7.8) && reg[0].getInitIndex() == 10 &&
          approxEqual(reg[1].getSeverity(), 7.8) && reg[1].getInitIndex() == 12 &&
          approxEqual(reg[5].getSeverity(), 2.1),
          "SeverityComparator(DESC) breaks equal-severity ties chronologically");
    check(samePointerMultiset(addressesBefore, reg, n),
          "Insertion Sort swaps strictly pointers in O(1) (identical heap addresses, zero slicing)");

    // Chronological ascending via relational Quicksort (uses DetectedEvent::operator< directly)
    EventSorter::quickSort(reg);
    check(EventSorter::isSorted(reg) && EventSorter::isSorted(reg, *comparators[1]),
          "Relational Quicksort matches ChronologicalComparator(ASC)");
    check(reg[0].getInitIndex() == 5 && reg[1].getInitIndex() == 10 &&
          reg[2].getInitIndex() == 12 && reg[5].getInitIndex() == 50,
          "Relational Quicksort produces exact chronological sequence");
    check(samePointerMultiset(addressesBefore, reg, n),
          "Relational Quicksort swaps strictly pointers (identical heap addresses, zero slicing)");

    // Duration descending via Functor Quicksort
    EventSorter::quickSort(reg, *comparators[2]);
    check(EventSorter::isSorted(reg, *comparators[2]),
          "Functor Quicksort with DurationComparator(DESC) orders correctly");
    check(approxEqual(reg[0].getDurationInSecs(), 0.50) &&
          approxEqual(reg[1].getDurationInSecs(), 0.20) &&
          approxEqual(reg[2].getDurationInSecs(), 0.15) &&
          approxEqual(reg[5].getDurationInSecs(), 0.0),
          "DurationComparator(DESC) ranks sustained anomalies ahead of point peaks");

    // Channel grouping via Insertion Sort (AF4 -> MLII -> V5, chronological inside each channel)
    EventSorter::insertionSort(reg, *comparators[3]);
    check(EventSorter::isSorted(reg, *comparators[3]),
          "Insertion Sort with ChannelComparator(ASC) orders correctly");
    check(reg[0].getCodeOfChannel() == "AF4" && reg[0].getInitIndex() == 12 &&
          reg[1].getCodeOfChannel() == "AF4" && reg[1].getInitIndex() == 30 &&
          reg[2].getCodeOfChannel() == "MLII" && reg[2].getInitIndex() == 10 &&
          reg[3].getCodeOfChannel() == "MLII" && reg[3].getInitIndex() == 40 &&
          reg[4].getCodeOfChannel() == "V5" && reg[4].getInitIndex() == 5 &&
          reg[5].getCodeOfChannel() == "V5" && reg[5].getInitIndex() == 50,
          "ChannelComparator(ASC) groups by channel and sorts chronologically within each group");

    // Reverse modes of all 4 comparators
    const SeverityComparator sevAsc(false);
    const ChronologicalComparator timeDesc(false);
    const DurationComparator durAsc(false);
    const ChannelComparator chDesc(false);

    EventSorter::quickSort(reg, sevAsc);
    check(EventSorter::isSorted(reg, sevAsc) && approxEqual(reg[0].getSeverity(), 2.1),
          "SeverityComparator(ASC) works via Functor Quicksort");
    EventSorter::insertionSort(reg, timeDesc);
    check(EventSorter::isSorted(reg, timeDesc) && reg[0].getInitIndex() == 50,
          "ChronologicalComparator(DESC) works via Insertion Sort");
    EventSorter::quickSort(reg, durAsc);
    check(EventSorter::isSorted(reg, durAsc) && approxEqual(reg[0].getDurationInSecs(), 0.0),
          "DurationComparator(ASC) works via Functor Quicksort");
    EventSorter::insertionSort(reg, chDesc);
    check(EventSorter::isSorted(reg, chDesc) && reg[0].getCodeOfChannel() == "V5",
          "ChannelComparator(DESC) works via Insertion Sort");

    // Explicit Anti-Slicing verification: every concrete subclass retains all derived attributes
    check(samePointerMultiset(addressesBefore, reg, n),
          "All 6 pointers remain exact original addresses after 8 consecutive sorts");

    int verifiedPeaks = 0;
    int verifiedThresholds = 0;
    int verifiedAnomalies = 0;
    for (int i = 0; i < n; ++i) {
        if (const auto* pk = dynamic_cast<const PeakEvent*>(&reg[i])) {
            if (approxEqual(pk->getPeakAmplitude(), -8.4) && !pk->isPositivePeak()) {
                ++verifiedPeaks;
            } else if (approxEqual(pk->getPeakAmplitude(), 6.5) && pk->isPositivePeak()) {
                ++verifiedPeaks;
            }
        } else if (const auto* th = dynamic_cast<const ThresholdEvent*>(&reg[i])) {
            if (th->getCrossingType() == ThresholdEvent::CrossingType::UPPER &&
                approxEqual(th->getReferenceThreshold(), 2.0)) {
                ++verifiedThresholds;
            } else if (th->getCrossingType() == ThresholdEvent::CrossingType::LOWER &&
                       approxEqual(th->getReferenceThreshold(), -1.0)) {
                ++verifiedThresholds;
            }
        } else if (const auto* an = dynamic_cast<const StatisticalAnomalyEvent*>(&reg[i])) {
            if (an->getInvolvedWindows() == 4 && approxEqual(an->getMaxZScore(), -5.5)) {
                ++verifiedAnomalies;
            } else if (an->getInvolvedWindows() == 2 && approxEqual(an->getEnergyRatio(), 30.25)) {
                ++verifiedAnomalies;
            }
        }
    }
    check(verifiedPeaks == 2 && verifiedThresholds == 2 && verifiedAnomalies == 2,
          "dynamic_cast confirms zero Object Slicing across PeakEvent, ThresholdEvent, and StatisticalAnomalyEvent");

    // Verify comparator names are non-empty and descriptive
    check(comparators[0]->getName() == "SeverityComparator(DESC)" &&
          comparators[1]->getName() == "ChronologicalComparator(ASC)" &&
          comparators[2]->getName() == "DurationComparator(DESC)" &&
          comparators[3]->getName() == "ChannelComparator(ASC)",
          "Polymorphic EventComparator::getName() returns canonical labels");

    for (int i = 0; i < 4; ++i) {
        delete comparators[i];
    }
}

void testFullEndToEndPipelineF1ToF4() {
    EventRegistry consolidated(0);
    {
        // Channel 1 (EEG): baseline ~10 uV, 1-sample impulsive artifact (900 uV) at idx 8,
        // and a sustained epileptic-like burst [20..27] at 60 uV
        double eegRaw[40];
        for (int i = 0; i < 40; ++i) {
            eegRaw[i] = (i % 2 == 0) ? 10.5 : 9.5;
        }
        eegRaw[8] = 900.0;  // Impulsive artifact -> must be suppressed by MedianFilter(3)
        for (int i = 20; i <= 27; ++i) {
            eegRaw[i] = 60.0;
        }

        // Channel 2 (ECG): baseline ~0 mV, two distinct R-peaks at indices 10 and 30
        double ecgRaw[40];
        for (int i = 0; i < 40; ++i) {
            ecgRaw[i] = (i % 2 == 0) ? 0.1 : -0.1;
        }
        ecgRaw[10] = 8.0;
        ecgRaw[11] = 8.0;
        ecgRaw[30] = 6.0;
        ecgRaw[31] = 6.0;

        ChannelSignal eeg("EEG_ICTAL", "uV", 100.0, eegRaw, 40);
        ChannelSignal ecg("ECG_MLII", "mV", 100.0, ecgRaw, 40);

        // Phase 2: MedianFilter(3) removes the 1-sample artifact at idx 8 while preserving sustained burst & 2-sample peaks
        ProcessingPipeline pipeline(2);
        pipeline.addStage(MedianFilter(3));
        check(eeg.applyPipeline(pipeline, "raw"), "Phase 2 pipeline cleans EEG channel");
        check(ecg.applyPipeline(pipeline, "raw"), "Phase 2 pipeline cleans ECG channel");
        check(eeg.getProcessedElement(8) < 15.0,
              "MedianFilter(3) eliminates 900 uV impulse artifact prior to Phase 3 detection");

        // Phase 3: Multi-detector engine scans both cleaned channels
        DetectionEngine engine(3);
        engine.addDetector(PeakDetector("peaks", 4.0, 2.0, 5, false, "processed"));
        engine.addDetector(ThresholdDetector("burst_thr", 30.0, 20.0, 3, ThresholdEvent::CrossingType::UPPER, "processed"));
        engine.addDetector(WindowAnomalyDetector("win_anom", 4, 2, 1.5, 2.5, "processed"));

        const ChannelSignal* channels[2] = {&eeg, &ecg};
        consolidated = engine.runMultiple(channels, 2, false);
    }
    // Signals are destroyed here; consolidated registry remains completely valid

    check(consolidated.getCount() >= 4,
          "F1->F2->F3 pipeline detects heterogeneous events across both cleaned channels");
    check(consolidated.countByType("PEAK") >= 2 &&
          consolidated.countByType("THRESHOLD") >= 1 &&
          consolidated.countByType("STATISTICAL_ANOMALY") >= 1,
          "Consolidated registry contains all three concrete event subclasses");

    // Phase 4: Triage with EventSorter (Quicksort & Functor Insertion Sort)
    const SeverityComparator bySevDesc(true);
    const DurationComparator byDurDesc(true);
    const ChannelComparator byChannelAsc(true);

    EventSorter::quickSort(consolidated, bySevDesc);
    check(EventSorter::isSorted(consolidated, bySevDesc),
          "Phase 4 Quicksort triages F1->F3 events by descending severity");

    EventSorter::insertionSort(consolidated, byDurDesc);
    check(EventSorter::isSorted(consolidated, byDurDesc) &&
          consolidated[0].getDurationInSecs() > 0.0,
          "Phase 4 Insertion Sort ranks sustained burst ahead of point peaks");

    EventSorter::insertionSort(consolidated, byChannelAsc);
    check(EventSorter::isSorted(consolidated, byChannelAsc) &&
          consolidated[0].getCodeOfChannel() == "ECG_MLII",
          "Phase 4 ChannelComparator groups ECG_MLII before EEG_ICTAL");

    EventSorter::quickSort(consolidated);
    check(EventSorter::isSorted(consolidated),
          "Phase 4 relational Quicksort restores global chronological timeline");
}

}  // namespace

int main() {
    testSortingBoundaryConditions();
    testPolymorphicFunctorsAndAntiSlicing();
    testFullEndToEndPipelineF1ToF4();

    std::cout << "Phase 4 Sorting, Functors & F1-F4 Integration Tests -> Passed: " << gPassed
              << " | Failed: " << gFailed << "\n";
    return (gFailed == 0) ? 0 : 1;
}
