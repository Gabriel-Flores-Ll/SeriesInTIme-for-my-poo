#ifndef BENCHMARK_SUITE_H
#define BENCHMARK_SUITE_H

#include <iosfwd>
#include <string>

/**
 * @struct MitBihBenchmarkResult
 * @brief Quantitative QRS detection metrics validated against PhysioNet MIT-BIH annotations.
 */
struct MitBihBenchmarkResult {
    bool executed;
    int totalSamples;
    int groundTruthBeats;
    int detectedPeaks;
    int truePositives;
    int falsePositives;
    int falseNegatives;
    double sensitivityPct;
    double precisionPct;
    double f1ScorePct;
    double meanLatencySamples;
    double meanLatencyMs;

    MitBihBenchmarkResult()
        : executed(false),
          totalSamples(0),
          groundTruthBeats(0),
          detectedPeaks(0),
          truePositives(0),
          falsePositives(0),
          falseNegatives(0),
          sensitivityPct(0.0),
          precisionPct(0.0),
          f1ScorePct(0.0),
          meanLatencySamples(0.0),
          meanLatencyMs(0.0) {}
};

/**
 * @struct UciBenchmarkResult
 * @brief Quantitative impulse-noise suppression and multi-channel anomaly metrics for UCI EEG Eye State.
 */
struct UciBenchmarkResult {
    bool executed;
    int loadedChannels;
    int samplesPerChannel;
    double af4RawMax;
    double af4RawStdDev;
    double af4MovingAvgMax;
    double af4MovingAvgStdDev;
    double af4MedianMax;
    double af4MedianStdDev;
    double stdDevReductionPct;
    int multiChannelEvents;

    UciBenchmarkResult()
        : executed(false),
          loadedChannels(0),
          samplesPerChannel(0),
          af4RawMax(0.0),
          af4RawStdDev(0.0),
          af4MovingAvgMax(0.0),
          af4MovingAvgStdDev(0.0),
          af4MedianMax(0.0),
          af4MedianStdDev(0.0),
          stdDevReductionPct(0.0),
          multiChannelEvents(0) {}
};

/**
 * @struct BonnBenchmarkResult
 * @brief Quantitative healthy vs. inter-ictal vs. epileptic ictal triage metrics for Bonn EEG.
 */
struct BonnBenchmarkResult {
    bool executed;
    int loadedSets;
    bool hasHealthySet;
    bool hasIctalSet;
    bool ictalRankedFirst;
    int totalEvents;
    std::string topSeverityChannel;
    double topSeverityScore;

    BonnBenchmarkResult()
        : executed(false),
          loadedSets(0),
          hasHealthySet(false),
          hasIctalSet(false),
          ictalRankedFirst(false),
          totalEvents(0),
          topSeverityChannel(""),
          topSeverityScore(0.0) {}
};

/**
 * @class BenchmarkSuite
 * @brief Non-instantiable Phase 6 scientific validation engine executing end-to-end
 *        multi-phase experiments (F1 -> F2 -> F3 -> F4 -> F5) on the three reference
 *        biomedical datasets: MIT-BIH Arrhythmia, UCI EEG Eye State, and Bonn University EEG.
 *        Uses strictly manual RAII memory management without STL containers.
 */
class BenchmarkSuite {
private:
    static bool fileExists(const std::string& path);
    static std::string resolveFirstExisting(const std::string* candidates, int count);
    static void printBanner(std::ostream& os, const std::string& title);

public:
    BenchmarkSuite() = delete;

    /**
     * @brief Runs the Pan-Tompkins-inspired QRS detection benchmark on MIT-BIH ECG data
     *        and validates detected peak indices against PhysioNet clinical beat annotations.
     * @param signalCsvPath Path to ECG CSV (default: data/mitbih_100_10s.csv).
     * @param annotationsCsvPath Path to beat annotations CSV (default: data/mitbih_100_10s_annotations.csv).
     * @param os Output stream for summary and 80-column report.
     * @param exportReportPath Optional file path to export the 80-column report (empty = skip).
     * @param exportSessionPath Optional file path to save the StudySession snapshot (empty = skip).
     * @param maxSamples Optional cap on samples to load (0 = load full file).
     * @return Populated MitBihBenchmarkResult with TP, FP, FN, Sensitivity, Precision, F1, and latency.
     */
    static MitBihBenchmarkResult runMitBihBenchmark(
        const std::string& signalCsvPath,
        const std::string& annotationsCsvPath,
        std::ostream& os,
        const std::string& exportReportPath = "",
        const std::string& exportSessionPath = "",
        int maxSamples = 0);

    /**
     * @brief Runs the 14-channel UCI EEG Eye State benchmark demonstrating MedianFilter vs.
     *        MovingAverageFilter impulse-artifact elimination (AF4/P7 spikes) and multi-channel triage.
     * @param uciCsvPath Path to UCI CSV (e.g., data/uci_eeg_eyestate_2000.csv or full CSV).
     * @param os Output stream for comparative statistics and 80-column report.
     * @param exportReportPath Optional file path to export the 80-column report (empty = skip).
     * @param exportSessionPath Optional file path to save the StudySession snapshot (empty = skip).
     * @param maxSamples Optional cap on rows to load (0 = load full file).
     * @return Populated UciBenchmarkResult with before/after stdDev reduction and event counts.
     */
    static UciBenchmarkResult runUciEyeStateBenchmark(
        const std::string& uciCsvPath,
        std::ostream& os,
        const std::string& exportReportPath = "",
        const std::string& exportSessionPath = "",
        int maxSamples = 0);

    /**
     * @brief Runs the Bonn University EEG benchmark contrasting available healthy (Z/O),
     *        inter-ictal (N/F), and epileptic ictal (S) recordings via Quicksort severity triage.
     * @param healthyPath Path to Set A healthy recording (e.g., data/Z001.txt).
     * @param interIctalPath Optional path to Set D inter-ictal recording (e.g., data/F001.txt).
     * @param ictalPath Optional path to Set E epileptic ictal recording (e.g., data/S001.txt).
     * @param os Output stream for ranking results and 80-column report.
     * @param exportReportPath Optional file path to export the 80-column report (empty = skip).
     * @param exportSessionPath Optional file path to save the StudySession snapshot (empty = skip).
     * @return Populated BonnBenchmarkResult indicating whether Set E ranked #1 in severity.
     */
    static BonnBenchmarkResult runBonnEegBenchmark(
        const std::string& healthyPath,
        const std::string& interIctalPath,
        const std::string& ictalPath,
        std::ostream& os,
        const std::string& exportReportPath = "",
        const std::string& exportSessionPath = "");

    /**
     * @brief Auto-discovers available datasets under data/ (and data/bonn/) and executes
     *        all three scientific benchmarks, printing consolidated tables and optional reports.
     * @param os Output stream for benchmark reports.
     * @param reportDir Optional directory prefix for exported reports (empty = no file export).
     * @return Number of benchmarks successfully executed (0..3).
     */
    static int runAllDefaultBenchmarks(std::ostream& os, const std::string& reportDir = "");
};

#endif
