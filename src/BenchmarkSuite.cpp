#include "BenchmarkSuite.h"

#include "ChannelComparator.h"
#include "ChannelSignal.h"
#include "ChannelStadistics.h"
#include "ChronologicalComparator.h"
#include "DetectecEvent.h"
#include "DifferentialFilter.h"
#include "EventRegistry.h"
#include "EventSorter.h"
#include "MedianFilter.h"
#include "MovingAverageFilter.h"
#include "PeakDetector.h"
#include "PersistenceManager.h"
#include "ProcessingPipeline.h"
#include "SeverityComparator.h"
#include "StudySession.h"
#include "ThresholdDetector.h"
#include "ThresholdEvent.h"
#include "WaveRectifier.h"
#include "WindowAnomalyDetector.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <stdexcept>

bool BenchmarkSuite::fileExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    std::ifstream in(path);
    return in.good();
}

std::string BenchmarkSuite::resolveFirstExisting(const std::string* candidates, int count) {
    for (int i = 0; i < count; ++i) {
        if (fileExists(*(candidates + i))) {
            return *(candidates + i);
        }
    }
    return "";
}

void BenchmarkSuite::printBanner(std::ostream& os, const std::string& title) {
    const std::string rule(80, '=');
    os << "\n" << rule << "\n";
    os << "  " << title << "\n";
    os << rule << "\n";
}

MitBihBenchmarkResult BenchmarkSuite::runMitBihBenchmark(
    const std::string& signalCsvPath,
    const std::string& annotationsCsvPath,
    std::ostream& os,
    const std::string& exportReportPath,
    const std::string& exportSessionPath,
    int maxSamples) {
    MitBihBenchmarkResult result;
    printBanner(os, "BENCHMARK 1: MIT-BIH ARRHYTHMIA DATABASE (ECG QRS DETECTION)");

    if (!fileExists(signalCsvPath)) {
        os << "  [SKIP] Signal file not found: " << signalCsvPath << "\n";
        return result;
    }

    StudySession session("MIT_BIH_Record100_QRS", 2);
    const std::string codes[2] = {"MLII", "V5"};
    const int cols[2] = {1, 2};
    IngestionStats stats;

    PersistenceManager::loadMultiChannelCsv(
        signalCsvPath, session, codes, cols, 2, "mV", 360.0, true, ',', maxSamples, &stats);

    result.totalSamples = stats.loadedSamples;

    // Pan-Tompkins-inspired cascade on MLII (channel 0):
    // Median(3) -> TKEO DifferentialFilter -> FullWave Rectifier -> MovingAverage(11)
    session.getPipeline().addStage(MedianFilter(3));
    session.getPipeline().addStage(DifferentialFilter());
    session.getPipeline().addStage(WaveRectifier(RectifierMode::FullWave, 1.0));
    session.getPipeline().addStage(MovingAverageFilter(11));

    session.applyPipelineToChannel(0, "raw");

    // Adaptive thresholds derived directly from MLII's conditioned TKEO-envelope baseline
    const ChannelStadistics& envStats = session.getChannel(0).getProcessedStatistics();
    const double adaptiveAmp = envStats.getAverage() + 1.5 * envStats.getStandardDesviation();
    const double adaptiveProm = 1.25 * envStats.getStandardDesviation();
    const int refractorySamples = 90;  // 250 ms at 360 Hz

    // Keep MLII only for clean QRS benchmarking against MLII ground-truth annotations
    StudySession qrsSession("MIT_BIH_MLII_Validation", 1);
    qrsSession.addChannel(session.getChannel(0));
    qrsSession.getPipeline() = session.getPipeline();
    qrsSession.getEngine().addDetector(
        PeakDetector("qrs_pan_tompkins", adaptiveAmp, adaptiveProm, refractorySamples, false, "processed"));

    qrsSession.runAnalysis(false);  // Sort chronologically via Quicksort
    const ChronologicalComparator byTimeAsc(true);
    qrsSession.sortEvents(byTimeAsc, true);

    const EventRegistry& reg = qrsSession.getRegistry();
    result.detectedPeaks = reg.getCount();
    result.executed = true;

    // Validate against clinical beat annotations if available
    int* gtBeats = nullptr;
    int gtCount = 0;
    if (!annotationsCsvPath.empty() &&
        PersistenceManager::loadMitBihBeatAnnotations(annotationsCsvPath, gtBeats, gtCount, true) &&
        gtCount > 0) {
        result.groundTruthBeats = gtCount;
        bool* matchedGt = new bool[gtCount]();
        const int toleranceSamples = 36;  // +/- 100 ms (stricter than AAMI +/- 150 ms)

        double totalAbsLatency = 0.0;
        for (int p = 0; p < reg.getCount(); ++p) {
            const int peakIdx = reg[p].getInitIndex();
            int bestGt = -1;
            int bestDist = toleranceSamples + 1;

            for (int g = 0; g < gtCount; ++g) {
                if (*(matchedGt + g)) {
                    continue;
                }
                const int dist = std::abs(peakIdx - *(gtBeats + g));
                if (dist <= toleranceSamples && dist < bestDist) {
                    bestDist = dist;
                    bestGt = g;
                }
            }

            if (bestGt >= 0) {
                *(matchedGt + bestGt) = true;
                ++result.truePositives;
                totalAbsLatency += static_cast<double>(bestDist);
            } else {
                ++result.falsePositives;
            }
        }

        result.falseNegatives = gtCount - result.truePositives;
        if (gtCount > 0) {
            result.sensitivityPct = 100.0 * static_cast<double>(result.truePositives) /
                                    static_cast<double>(gtCount);
        }
        if (result.detectedPeaks > 0) {
            result.precisionPct = 100.0 * static_cast<double>(result.truePositives) /
                                  static_cast<double>(result.detectedPeaks);
        }
        if (result.sensitivityPct + result.precisionPct > 0.0) {
            result.f1ScorePct = 2.0 * result.sensitivityPct * result.precisionPct /
                                (result.sensitivityPct + result.precisionPct);
        }
        if (result.truePositives > 0) {
            result.meanLatencySamples = totalAbsLatency / static_cast<double>(result.truePositives);
            result.meanLatencyMs = (result.meanLatencySamples / 360.0) * 1000.0;
        }

        delete[] matchedGt;
    }
    delete[] gtBeats;

    const std::ios::fmtflags savedFlags = os.flags();
    const std::streamsize savedPrec = os.precision();

    os << std::fixed << std::setprecision(2);
    os << "  Signal file      : " << signalCsvPath << " (" << result.totalSamples
       << " samples @ 360 Hz, skipped=" << stats.skippedLines << ")\n";
    os << "  Detected QRS     : " << result.detectedPeaks << "\n";
    if (result.groundTruthBeats > 0) {
        os << "  Ground Truth     : " << result.groundTruthBeats
           << " clinical beats (non-beat '+' markers excluded)\n";
        os << "  Validation (+/-100 ms): TP=" << result.truePositives
           << " | FP=" << result.falsePositives
           << " | FN=" << result.falseNegatives << "\n";
        os << "  Metrics          : Sensitivity=" << result.sensitivityPct
           << "% | Precision=" << result.precisionPct
           << "% | F1-Score=" << result.f1ScorePct << "%\n";
        os << "  Mean Causal Lag  : " << result.meanLatencySamples
           << " samples (" << result.meanLatencyMs << " ms)\n";
    } else {
        os << "  Ground Truth     : (annotations file not provided/found)\n";
    }

    os.flags(savedFlags);
    os.precision(savedPrec);

    PersistenceManager::writeReport(qrsSession, os, 20);

    if (!exportReportPath.empty()) {
        PersistenceManager::exportReport(qrsSession, exportReportPath, 0);
        os << "  [SAVED] 80-column report exported to: " << exportReportPath << "\n";
    }
    if (!exportSessionPath.empty()) {
        PersistenceManager::saveSession(qrsSession, exportSessionPath);
        os << "  [SAVED] StudySession persisted to: " << exportSessionPath << "\n";
    }

    return result;
}

UciBenchmarkResult BenchmarkSuite::runUciEyeStateBenchmark(
    const std::string& uciCsvPath,
    std::ostream& os,
    const std::string& exportReportPath,
    const std::string& exportSessionPath,
    int maxSamples) {
    UciBenchmarkResult result;
    printBanner(os, "BENCHMARK 2: UCI EEG EYE STATE (14-CHANNEL IMPULSE SUPPRESSION & TRIAGE)");

    if (!fileExists(uciCsvPath)) {
        os << "  [SKIP] UCI EEG file not found: " << uciCsvPath << "\n";
        return result;
    }

    static const std::string kChannelCodes[14] = {
        "AF3", "F7", "F3", "FC5", "T7", "P7", "O1",
        "O2",  "P8", "T8", "FC6", "F4", "F8", "AF4"
    };
    static const int kChannelCols[14] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    StudySession session("UCI_EEG_EyeState_14ch", 14);
    IngestionStats stats;
    const int loaded = PersistenceManager::loadMultiChannelCsv(
        uciCsvPath, session, kChannelCodes, kChannelCols, 14, "uV", 128.0, false, ',', maxSamples, &stats);

    result.loadedChannels = loaded;
    result.samplesPerChannel = stats.loadedSamples;

    // Controlled A/B comparison on channel AF4 (index 13) before global MedianFilter pass
    const int af4Idx = session.findChannelByCode("AF4");
    if (af4Idx >= 0) {
        ChannelSignal af4Probe(session.getChannel(af4Idx));
        const ChannelStadistics& rawSt = af4Probe.getProcessedStatistics();
        result.af4RawMax = rawSt.getMaximum();
        result.af4RawStdDev = rawSt.getStandardDesviation();

        MovingAverageFilter ma5(5);
        af4Probe.applyModule(ma5, "raw");
        const ChannelStadistics& maSt = af4Probe.getProcessedStatistics();
        result.af4MovingAvgMax = maSt.getMaximum();
        result.af4MovingAvgStdDev = maSt.getStandardDesviation();

        MedianFilter med5(5);
        af4Probe.applyModule(med5, "raw");
        const ChannelStadistics& medSt = af4Probe.getProcessedStatistics();
        result.af4MedianMax = medSt.getMaximum();
        result.af4MedianStdDev = medSt.getStandardDesviation();

        if (result.af4RawStdDev > 1e-12) {
            result.stdDevReductionPct =
                100.0 * (1.0 - (result.af4MedianStdDev / result.af4RawStdDev));
        }
    }

    // Apply MedianFilter(5) to all 14 EEG channels cleanly
    session.getPipeline().addStage(MedianFilter(5));
    session.applyPipelineToAll("raw");

    // Detect multi-channel eye-blink / transient bursts with 0.25 s sliding windows (W=32, S=16)
    session.getEngine().addDetector(
        WindowAnomalyDetector("eeg_blink_burst", 32, 16, 2.5, 2.2, "processed"));

    result.multiChannelEvents = session.runAnalysis(true);  // Quicksort descending by severity

    // Demonstrate Unit 7 ChannelComparator via Insertion Sort, then restore Severity triage
    const ChannelComparator byChannelAsc(true);
    session.sortEvents(byChannelAsc, false);
    const SeverityComparator bySevDesc(true);
    session.sortEvents(bySevDesc, true);

    result.executed = true;

    const std::ios::fmtflags savedFlags = os.flags();
    const std::streamsize savedPrec = os.precision();

    os << std::fixed << std::setprecision(2);
    os << "  Dataset file     : " << uciCsvPath << " (" << result.loadedChannels
       << " channels x " << result.samplesPerChannel
       << " samples @ 128 Hz, skipped=" << stats.skippedLines << ")\n";
    os << "  AF4 Impulse Proof (MedianFilter vs MovingAverageFilter):\n";
    os << "    RAW            : Max=" << std::setw(10) << result.af4RawMax
       << " uV | StdDev=" << std::setw(9) << result.af4RawStdDev << " uV\n";
    os << "    MovingAvg(W=5) : Max=" << std::setw(10) << result.af4MovingAvgMax
       << " uV | StdDev=" << std::setw(9) << result.af4MovingAvgStdDev << " uV (smears spike)\n";
    os << "    Median(W=5)    : Max=" << std::setw(10) << result.af4MedianMax
       << " uV | StdDev=" << std::setw(9) << result.af4MedianStdDev
       << " uV (-" << result.stdDevReductionPct << "% StdDev)\n";
    os << "  Events Detected  : " << result.multiChannelEvents
       << " multi-channel bursts (triaged by SeverityComparator via Quicksort)\n";

    os.flags(savedFlags);
    os.precision(savedPrec);

    PersistenceManager::writeReport(session, os, 15);

    if (!exportReportPath.empty()) {
        PersistenceManager::exportReport(session, exportReportPath, 50);
        os << "  [SAVED] 80-column report exported to: " << exportReportPath << "\n";
    }
    if (!exportSessionPath.empty()) {
        PersistenceManager::saveSession(session, exportSessionPath);
        os << "  [SAVED] StudySession persisted to: " << exportSessionPath << "\n";
    }

    return result;
}

BonnBenchmarkResult BenchmarkSuite::runBonnEegBenchmark(
    const std::string& healthyPath,
    const std::string& interIctalPath,
    const std::string& ictalPath,
    std::ostream& os,
    const std::string& exportReportPath,
    const std::string& exportSessionPath) {
    BonnBenchmarkResult result;
    printBanner(os, "BENCHMARK 3: BONN UNIVERSITY EEG (HEALTHY VS. INTER-ICTAL VS. ICTAL)");

    StudySession session("Bonn_EEG_Epilepsy_Triage", 3);

    if (fileExists(healthyPath)) {
        ChannelSignal z = PersistenceManager::loadSingleColumnTxt(
            healthyPath, "BONN_A_Z001", "uV", 173.61);
        session.addChannel(z);
        result.hasHealthySet = true;
        ++result.loadedSets;
    }

    if (fileExists(interIctalPath)) {
        ChannelSignal f = PersistenceManager::loadSingleColumnTxt(
            interIctalPath, "BONN_D_F001", "uV", 173.61);
        session.addChannel(f);
        ++result.loadedSets;
    }

    if (fileExists(ictalPath)) {
        ChannelSignal s = PersistenceManager::loadSingleColumnTxt(
            ictalPath, "BONN_E_S001", "uV", 173.61);
        session.addChannel(s);
        result.hasIctalSet = true;
        ++result.loadedSets;
    }

    if (result.loadedSets == 0) {
        os << "  [SKIP] No Bonn EEG files found.\n";
        os << "         Place Z001.txt (Set A healthy), F001.txt (Set D inter-ictal), and\n";
        os << "         S001.txt (Set E epileptic ictal) inside data/ or data/bonn/ to run.\n";
        return result;
    }

    // Condition signals: MedianFilter(3) removes single-sample telemetry glitches;
    // FullWave Rectifier converts bipolar spikes into strictly non-negative amplitudes.
    session.getPipeline().addStage(MedianFilter(3));
    session.getPipeline().addStage(WaveRectifier(RectifierMode::FullWave, 1.0));
    session.applyPipelineToAll("raw");

    // Clinical EEG thresholds: healthy scalp EEG (Set A) stays < 200 uV,
    // inter-ictal sharp transients (Set D) are brief, whereas epileptic ictal
    // discharges (Set E) produce sustained high-amplitude bursts (> 280 uV)
    // and extreme sliding-window energy surges.
    session.getEngine().addDetector(
        PeakDetector("ictal_spike", 280.0, 150.0, 15, false, "processed"));
    session.getEngine().addDetector(
        ThresholdDetector("epileptic_burst", 250.0, 150.0, 3,
                          ThresholdEvent::CrossingType::UPPER, "processed"));
    session.getEngine().addDetector(
        WindowAnomalyDetector("ictal_energy", 174, 87, 2.5, 3.0, "processed"));

    result.totalEvents = session.runAnalysis(true);  // Quicksort descending by severity
    result.executed = true;

    const EventRegistry& reg = session.getRegistry();
    if (!reg.isEmpty()) {
        result.topSeverityChannel = reg[0].getCodeOfChannel();
        result.topSeverityScore = reg[0].getSeverity();
        result.ictalRankedFirst = (result.topSeverityChannel == "BONN_E_S001");
    }

    const std::ios::fmtflags savedFlags = os.flags();
    const std::streamsize savedPrec = os.precision();

    os << std::fixed << std::setprecision(3);
    os << "  Loaded Bonn Sets : " << result.loadedSets
       << " (Healthy=" << (result.hasHealthySet ? "YES" : "NO")
       << ", Ictal_SetE=" << (result.hasIctalSet ? "YES" : "NO") << ")\n";
    os << "  Events Detected  : " << result.totalEvents << "\n";

    if (!result.topSeverityChannel.empty()) {
        os << "  Top #1 Severity  : " << result.topSeverityChannel
           << " (severity=" << result.topSeverityScore << ")\n";
    }
    if (result.hasHealthySet && result.hasIctalSet) {
        os << "  Ground-Truth Check (Set E ranked #1): "
           << (result.ictalRankedFirst ? "PASS" : "CHECK THRESHOLDS") << "\n";
    } else if (result.hasHealthySet && !result.hasIctalSet) {
        os << "  Note             : Only healthy baseline loaded; add data/S001.txt (Set E)\n"
           << "                     to run the full healthy-vs-epileptic ranking benchmark.\n";
    }

    os.flags(savedFlags);
    os.precision(savedPrec);

    PersistenceManager::writeReport(session, os, 15);

    if (!exportReportPath.empty()) {
        PersistenceManager::exportReport(session, exportReportPath, 50);
        os << "  [SAVED] 80-column report exported to: " << exportReportPath << "\n";
    }
    if (!exportSessionPath.empty()) {
        PersistenceManager::saveSession(session, exportSessionPath);
        os << "  [SAVED] StudySession persisted to: " << exportSessionPath << "\n";
    }

    return result;
}

int BenchmarkSuite::runAllDefaultBenchmarks(std::ostream& os, const std::string& reportDir) {
    int executedCount = 0;

    const std::string mitSignal = "data/mitbih_100_10s.csv";
    const std::string mitAnn = "data/mitbih_100_10s_annotations.csv";

    const std::string uciCandidates[2] = {
        "data/uci_eeg_eyestate_2000.csv",
        "data/uci_eeg_eyestate_full.csv"
    };
    const std::string uciPath = resolveFirstExisting(uciCandidates, 2);

    const std::string bonnZCandidates[2] = {"data/Z001.txt", "data/bonn/Z001.txt"};
    const std::string bonnFCandidates[2] = {"data/F001.txt", "data/bonn/F001.txt"};
    const std::string bonnSCandidates[2] = {"data/S001.txt", "data/bonn/S001.txt"};

    const std::string bonnZ = resolveFirstExisting(bonnZCandidates, 2);
    const std::string bonnF = resolveFirstExisting(bonnFCandidates, 2);
    const std::string bonnS = resolveFirstExisting(bonnSCandidates, 2);

    const std::string mitRep = reportDir.empty() ? "" : (reportDir + "/report_mitbih.txt");
    const std::string uciRep = reportDir.empty() ? "" : (reportDir + "/report_uci_eeg.txt");
    const std::string bonnRep = reportDir.empty() ? "" : (reportDir + "/report_bonn_eeg.txt");

    const MitBihBenchmarkResult mitRes =
        runMitBihBenchmark(mitSignal, mitAnn, os, mitRep, "", 0);
    if (mitRes.executed) {
        ++executedCount;
    }

    const UciBenchmarkResult uciRes =
        runUciEyeStateBenchmark(uciPath, os, uciRep, "", 0);
    if (uciRes.executed) {
        ++executedCount;
    }

    const BonnBenchmarkResult bonnRes =
        runBonnEegBenchmark(bonnZ, bonnF, bonnS, os, bonnRep, "");
    if (bonnRes.executed) {
        ++executedCount;
    }

    return executedCount;
}
