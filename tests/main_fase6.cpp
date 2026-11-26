#include "BenchmarkSuite.h"
#include "PersistenceManager.h"
#include "StudySession.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int gPassed = 0;
int gFailed = 0;

void check(bool condition, const char* label) {
    if (condition) {
        ++gPassed;
    } else {
        ++gFailed;
        std::cerr << "[FAIL] " << label << "\n";
    }
}

bool fileExists(const std::string& path) {
    std::ifstream in(path);
    return in.good();
}

void testControlledMitBihBenchmark() {
    // Deterministic 3-second synthetic ECG at 360 Hz (1080 samples) with 4 clean QRS spikes
    const std::string sigFile = "/tmp/test_fase6_mit.csv";
    const std::string annFile = "/tmp/test_fase6_mit_ann.csv";
    const std::string repFile = "/tmp/test_fase6_mit_rep.txt";
    const std::string sesFile = "/tmp/test_fase6_mit.dat";

    const int beatIdx[4] = {120, 410, 700, 990};
    {
        std::ofstream out(sigFile);
        out << "sample_idx,MLII_mV,V5_mV\n";
        for (int i = 0; i < 1080; ++i) {
            double v = -0.15 + 0.02 * std::sin(static_cast<double>(i) * 0.05);
            for (int b = 0; b < 4; ++b) {
                const int d = i - beatIdx[b];
                if (d == -2 || d == 2) v -= 0.15;
                if (d == -1 || d == 1) v += 0.65;
                if (d == 0) v += 1.45;
            }
            out << i << "," << v << "," << (v * 0.5) << "\n";
        }
    }
    {
        std::ofstream out(annFile);
        out << "sample_idx,symbol\n";
        out << "18,+\n";  // Must be filtered out as non-beat rhythm marker
        for (int b = 0; b < 4; ++b) {
            out << beatIdx[b] << ",N\n";
        }
    }

    std::ostringstream sink;
    const MitBihBenchmarkResult res =
        BenchmarkSuite::runMitBihBenchmark(sigFile, annFile, sink, repFile, sesFile);

    check(res.executed && res.totalSamples == 1080,
          "Controlled MIT-BIH benchmark loads all 1080 samples");
    check(res.groundTruthBeats == 4,
          "Controlled MIT-BIH benchmark filters '+' marker and counts 4 clinical beats");
    check(res.truePositives == 4 && res.falsePositives == 0 && res.falseNegatives == 0,
          "Controlled MIT-BIH benchmark achieves 100% TP (4/4), 0 FP, 0 FN");
    check(res.f1ScorePct > 99.9 && res.meanLatencySamples < 15.0,
          "Controlled MIT-BIH benchmark reports F1=100% and low causal lag");
    check(fileExists(repFile) && fileExists(sesFile),
          "Controlled MIT-BIH benchmark exports both 80-col report and StudySession snapshot");

    // Verify exported StudySession reloads cleanly
    StudySession reloaded("Empty", 0);
    PersistenceManager::loadSession(sesFile, reloaded);
    check(reloaded.getRegistry().getCount() == 4 && reloaded.getPipeline().getCount() == 4,
          "Exported MIT-BIH StudySession reloads polymorphically with all 4 stages & 4 QRS events");

    std::remove(sigFile.c_str());
    std::remove(annFile.c_str());
    std::remove(repFile.c_str());
    std::remove(sesFile.c_str());
}

void testControlledBonnRankingBenchmark() {
    // Healthy Set A (~40 uV) vs Inter-ictal Set D (~110 uV) vs Ictal Set E (~650 uV spikes)
    const std::string zFile = "/tmp/test_fase6_bonn_Z.txt";
    const std::string fFile = "/tmp/test_fase6_bonn_F.txt";
    const std::string sFile = "/tmp/test_fase6_bonn_S.txt";
    const std::string repFile = "/tmp/test_fase6_bonn_rep.txt";

    {
        std::ofstream oz(zFile), of(fFile), os(sFile);
        for (int i = 0; i < 800; ++i) {
            const double base = 30.0 * std::sin(static_cast<double>(i) * 0.15);
            oz << static_cast<int>(base) << "\r\n";

            double inter = 60.0 * std::sin(static_cast<double>(i) * 0.18);
            if (i == 350 || i == 351) inter = 210.0;
            of << static_cast<int>(inter) << "\r\n";

            double ictal = 80.0 * std::sin(static_cast<double>(i) * 0.25);
            if ((i >= 200 && i <= 206) || (i >= 520 && i <= 528)) {
                ictal = 750.0;
            }
            os << static_cast<int>(ictal) << "\r\n";
        }
    }

    std::ostringstream sink;
    const BonnBenchmarkResult res =
        BenchmarkSuite::runBonnEegBenchmark(zFile, fFile, sFile, sink, repFile, "");

    check(res.executed && res.loadedSets == 3 && res.hasHealthySet && res.hasIctalSet,
          "Controlled Bonn benchmark loads all 3 clinical sets (A, D, E)");
    check(res.totalEvents > 0 && res.ictalRankedFirst && res.topSeverityChannel == "BONN_E_S001",
          "Controlled Bonn benchmark ranks epileptic Set E (BONN_E_S001) #1 in severity via Quicksort");
    check(fileExists(repFile), "Controlled Bonn benchmark exports 80-column report");

    std::remove(zFile.c_str());
    std::remove(fFile.c_str());
    std::remove(sFile.c_str());
    std::remove(repFile.c_str());
}

void testRealProjectDatasetsIfAvailable() {
    std::ostringstream sink;

    if (fileExists("data/mitbih_100_10s.csv") && fileExists("data/mitbih_100_10s_annotations.csv")) {
        const MitBihBenchmarkResult mit = BenchmarkSuite::runMitBihBenchmark(
            "data/mitbih_100_10s.csv", "data/mitbih_100_10s_annotations.csv", sink);
        check(mit.executed && mit.totalSamples == 3600 && mit.groundTruthBeats == 13,
              "Real MIT-BIH Record 100 benchmark loads 3600 samples and 13 ground-truth beats");
        check(mit.truePositives >= 12 && mit.f1ScorePct >= 90.0,
              "Real MIT-BIH Record 100 benchmark achieves >= 90% F1 QRS detection accuracy");
    }

    if (fileExists("data/uci_eeg_eyestate_2000.csv")) {
        const UciBenchmarkResult uci = BenchmarkSuite::runUciEyeStateBenchmark(
            "data/uci_eeg_eyestate_2000.csv", sink);
        check(uci.executed && uci.loadedChannels == 14 && uci.samplesPerChannel == 2000,
              "Real UCI EEG Eye State benchmark loads all 14 channels x 2000 samples");
        check(uci.af4RawMax > 700000.0 && uci.af4MedianMax < 5000.0 && uci.stdDevReductionPct > 99.0,
              "Real UCI EEG Eye State proves >99% StdDev reduction on AF4 via MedianFilter(5)");
        check(uci.multiChannelEvents > 0,
              "Real UCI EEG Eye State detects multi-channel transient bursts after cleaning");
    }
}

}  // namespace

int main() {
    testControlledMitBihBenchmark();
    testControlledBonnRankingBenchmark();
    testRealProjectDatasetsIfAvailable();

    std::cout << "Phase 6 Console & Real-Dataset Benchmark Tests -> Passed: " << gPassed
              << " | Failed: " << gFailed << "\n";
    return (gFailed == 0) ? 0 : 1;
}
