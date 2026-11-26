#include "BenchmarkSuite.h"
#include "ChannelComparator.h"
#include "ChannelSignal.h"
#include "ChronologicalComparator.h"
#include "DifferentialFilter.h"
#include "DurationComparator.h"
#include "EventComparator.h"
#include "MedianFilter.h"
#include "MovingAverageFilter.h"
#include "PeakDetector.h"
#include "PersistenceManager.h"
#include "ProcessingPipeline.h"
#include "SeverityComparator.h"
#include "StudySession.h"
#include "ThresholdDetector.h"
#include "ThresholdEvent.h"
#include "ThresholdLimiter.h"
#include "WaveRectifier.h"
#include "WindowAnomalyDetector.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

bool readLine(const std::string& prompt, std::string& outLine) {
    std::cout << prompt;
    if (!std::getline(std::cin, outLine)) {
        return false;
    }
    return true;
}

bool readInt(const std::string& prompt, int& outValue) {
    std::string line;
    if (!readLine(prompt, line)) {
        return false;
    }
    std::istringstream iss(line);
    int val = 0;
    char trailing = '\0';
    if (!(iss >> val) || (iss >> trailing)) {
        std::cout << "  [ERROR] Invalid integer input.\n";
        return false;
    }
    outValue = val;
    return true;
}

bool readDouble(const std::string& prompt, double& outValue) {
    std::string line;
    if (!readLine(prompt, line)) {
        return false;
    }
    std::istringstream iss(line);
    double val = 0.0;
    char trailing = '\0';
    if (!(iss >> val) || (iss >> trailing) || !std::isfinite(val)) {
        std::cout << "  [ERROR] Invalid numeric input.\n";
        return false;
    }
    outValue = val;
    return true;
}

void printMainMenu(const StudySession& session) {
    std::cout << "\n================================================================================\n";
    std::cout << "  POLYMORPHIC TIME-SERIES PROCESSING ENGINE (C++17 / ZERO-STL / RAII)\n";
    std::cout << "================================================================================\n";
    std::cout << "  Active " << session << "\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "  1. Load Dataset into Active Session (TXT / CSV / Presets)\n";
    std::cout << "  2. Configure Processing Pipeline (5 Polymorphic Filters)\n";
    std::cout << "  3. Apply Pipeline / Restore Raw Signal Buffers\n";
    std::cout << "  4. Configure Detection Engine (3 Polymorphic Detectors)\n";
    std::cout << "  5. Run Detection Analysis & Sort/Triage Events (Unit 7 Functors)\n";
    std::cout << "  6. Print or Export 80-Column Study Report (Unit 6 Streams)\n";
    std::cout << "  7. Save or Load Complete StudySession State (Unit 6 Persistence)\n";
    std::cout << "  8. Run Real-Dataset Scientific Benchmarks (MIT-BIH / UCI / Bonn)\n";
    std::cout << "  9. Reset / Rename Active Session\n";
    std::cout << "  0. Exit\n";
    std::cout << "--------------------------------------------------------------------------------\n";
}

void handleLoadDataset(StudySession& session) {
    std::cout << "\n  --- Load Dataset ---\n";
    std::cout << "  1. Single-column TXT (e.g., Bonn Z001.txt / S001.txt)\n";
    std::cout << "  2. Single-column CSV (custom column index)\n";
    std::cout << "  3. Preset: MIT-BIH Record 100 (data/mitbih_100_10s.csv, MLII + V5)\n";
    std::cout << "  4. Preset: UCI EEG Eye State (data/uci_eeg_eyestate_2000.csv, 14 channels)\n";
    std::cout << "  0. Back\n";

    int sub = 0;
    if (!readInt("  Select option: ", sub) || sub == 0) {
        return;
    }

    if (sub == 1) {
        std::string path, code, unit;
        double fs = 173.61;
        int maxS = 0;
        if (!readLine("  File path: ", path) ||
            !readLine("  Channel code (no spaces): ", code) ||
            !readLine("  Unit (e.g. uV, mV): ", unit) ||
            !readDouble("  Sampling rate Hz (> 0): ", fs) ||
            !readInt("  Max samples (0 = all): ", maxS)) {
            return;
        }
        IngestionStats st;
        ChannelSignal sig = PersistenceManager::loadSingleColumnTxt(path, code, unit, fs, maxS, &st);
        session.addChannel(sig);
        std::cout << "  [OK] Loaded channel \"" << sig.getMetadata().getCodeOfChannel()
                  << "\" (" << st.loadedSamples << " samples, skipped=" << st.skippedLines << ").\n";
    } else if (sub == 2) {
        std::string path, code, unit;
        int col = 0, headerFlag = 1, maxS = 0;
        double fs = 100.0;
        if (!readLine("  CSV file path: ", path) ||
            !readInt("  Column index (0-based): ", col) ||
            !readInt("  Has header row? (1=yes, 0=no): ", headerFlag) ||
            !readLine("  Channel code (no spaces): ", code) ||
            !readLine("  Unit (e.g. mV, uV): ", unit) ||
            !readDouble("  Sampling rate Hz (> 0): ", fs) ||
            !readInt("  Max samples (0 = all): ", maxS)) {
            return;
        }
        IngestionStats st;
        ChannelSignal sig = PersistenceManager::loadCsvColumn(
            path, col, headerFlag != 0, code, unit, fs, ',', maxS, &st);
        session.addChannel(sig);
        std::cout << "  [OK] Loaded channel \"" << sig.getMetadata().getCodeOfChannel()
                  << "\" (" << st.loadedSamples << " samples, skipped=" << st.skippedLines << ").\n";
    } else if (sub == 3) {
        const std::string codes[2] = {"MLII", "V5"};
        const int cols[2] = {1, 2};
        IngestionStats st;
        const int n = PersistenceManager::loadMultiChannelCsv(
            "data/mitbih_100_10s.csv", session, codes, cols, 2, "mV", 360.0, true, ',', 0, &st);
        std::cout << "  [OK] Loaded " << n << " MIT-BIH channels ("
                  << st.loadedSamples << " samples each).\n";
    } else if (sub == 4) {
        static const std::string codes[14] = {
            "AF3", "F7", "F3", "FC5", "T7", "P7", "O1",
            "O2",  "P8", "T8", "FC6", "F4", "F8", "AF4"
        };
        static const int cols[14] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
        IngestionStats st;
        const int n = PersistenceManager::loadMultiChannelCsv(
            "data/uci_eeg_eyestate_2000.csv", session, codes, cols, 14, "uV", 128.0, false, ',', 0, &st);
        std::cout << "  [OK] Loaded " << n << " UCI EEG channels ("
                  << st.loadedSamples << " samples each).\n";
    }
}

void handleConfigurePipeline(StudySession& session) {
    ProcessingPipeline& pipe = session.getPipeline();
    std::cout << "\n  --- Configure Processing Pipeline ---\n";
    std::cout << pipe;
    std::cout << "  1. Add MovingAverageFilter (W)\n";
    std::cout << "  2. Add MedianFilter (W)\n";
    std::cout << "  3. Add DifferentialFilter (TKEO, W=3)\n";
    std::cout << "  4. Add WaveRectifier (FullWave/HalfWave + positive scale)\n";
    std::cout << "  5. Add ThresholdLimiter (min, max, scale)\n";
    std::cout << "  6. Toggle Stage Active/Bypass\n";
    std::cout << "  7. Remove Stage by Index\n";
    std::cout << "  8. Clear Pipeline\n";
    std::cout << "  0. Back\n";

    int sub = 0;
    if (!readInt("  Select option: ", sub) || sub == 0) {
        return;
    }

    if (sub == 1) {
        int w = 5;
        if (readInt("  Odd window size W (>= 3): ", w)) {
            pipe.addStage(MovingAverageFilter(w));
            std::cout << "  [OK] Added MovingAverageFilter.\n";
        }
    } else if (sub == 2) {
        int w = 5;
        if (readInt("  Odd window size W (>= 3): ", w)) {
            pipe.addStage(MedianFilter(w));
            std::cout << "  [OK] Added MedianFilter.\n";
        }
    } else if (sub == 3) {
        pipe.addStage(DifferentialFilter());
        std::cout << "  [OK] Added DifferentialFilter (TKEO).\n";
    } else if (sub == 4) {
        int modeChoice = 1;
        double scale = 1.0;
        if (readInt("  Mode (1=FullWave, 2=HalfWave): ", modeChoice) &&
            readDouble("  Positive scale factor (> 0): ", scale)) {
            const RectifierMode mode =
                (modeChoice == 2) ? RectifierMode::HalfWave : RectifierMode::FullWave;
            pipe.addStage(WaveRectifier(mode, scale));
            std::cout << "  [OK] Added WaveRectifier.\n";
        }
    } else if (sub == 5) {
        double mn = -1.0, mx = 1.0, scale = 1.0;
        if (readDouble("  Lower limit: ", mn) &&
            readDouble("  Upper limit (> lower): ", mx) &&
            readDouble("  Scale factor (!= 0): ", scale)) {
            pipe.addStage(ThresholdLimiter(mn, mx, scale));
            std::cout << "  [OK] Added ThresholdLimiter.\n";
        }
    } else if (sub == 6) {
        int idx = 0, state = 1;
        if (readInt("  Stage index: ", idx) &&
            readInt("  Active? (1=yes, 0=bypass): ", state)) {
            if (pipe.setStageActive(idx, state != 0)) {
                std::cout << "  [OK] Stage state updated.\n";
            } else {
                std::cout << "  [ERROR] Stage index out of range.\n";
            }
        }
    } else if (sub == 7) {
        int idx = 0;
        if (readInt("  Stage index to remove: ", idx)) {
            if (pipe.removeStage(idx)) {
                std::cout << "  [OK] Stage removed.\n";
            } else {
                std::cout << "  [ERROR] Stage index out of range.\n";
            }
        }
    } else if (sub == 8) {
        pipe.clear();
        std::cout << "  [OK] Pipeline cleared.\n";
    }
}

void handleApplyPipeline(StudySession& session) {
    if (!session.hasChannels()) {
        std::cout << "  [INFO] No channels loaded yet.\n";
        return;
    }

    std::cout << "\n  --- Apply Pipeline / Restore ---\n";
    std::cout << "  1. Apply pipeline to ALL channels from \"raw\"\n";
    std::cout << "  2. Apply pipeline to ALL channels from \"processed\"\n";
    std::cout << "  3. Restore ALL channels to immutable \"raw\" samples\n";
    std::cout << "  0. Back\n";

    int sub = 0;
    if (!readInt("  Select option: ", sub) || sub == 0) {
        return;
    }

    if (sub == 1) {
        session.applyPipelineToAll("raw");
        std::cout << "  [OK] Pipeline applied to all " << session.getChannelCount()
                  << " channels from raw buffer.\n";
    } else if (sub == 2) {
        session.applyPipelineToAll("processed");
        std::cout << "  [OK] Pipeline applied to all " << session.getChannelCount()
                  << " channels from processed buffer.\n";
    } else if (sub == 3) {
        session.restoreAllChannels();
        std::cout << "  [OK] All channels restored to raw samples.\n";
    }
}

void handleConfigureDetectors(StudySession& session) {
    DetectionEngine& eng = session.getEngine();
    std::cout << "\n  --- Configure Detection Engine ---\n";
    std::cout << eng;
    std::cout << "  1. Add PeakDetector\n";
    std::cout << "  2. Add ThresholdDetector\n";
    std::cout << "  3. Add WindowAnomalyDetector\n";
    std::cout << "  4. Clear All Detectors\n";
    std::cout << "  0. Back\n";

    int sub = 0;
    if (!readInt("  Select option: ", sub) || sub == 0) {
        return;
    }

    if (sub == 1) {
        std::string name;
        double minAmp = 1.0, minProm = 0.5;
        int minDist = 10, valleys = 0;
        if (readLine("  Detector name: ", name) &&
            readDouble("  Min amplitude (>= 0): ", minAmp) &&
            readDouble("  Min prominence (>= 0): ", minProm) &&
            readInt("  Min distance samples (>= 1): ", minDist) &&
            readInt("  Detect valleys too? (1=yes, 0=no): ", valleys)) {
            eng.addDetector(PeakDetector(name, minAmp, minProm, minDist, valleys != 0, "processed"));
            std::cout << "  [OK] Added PeakDetector.\n";
        }
    } else if (sub == 2) {
        std::string name;
        double act = 1.0, rel = 0.5;
        int minDur = 3, crossChoice = 1;
        if (readLine("  Detector name: ", name) &&
            readInt("  Crossing (1=UPPER, 2=LOWER): ", crossChoice) &&
            readDouble("  Activation threshold: ", act) &&
            readDouble("  Release threshold (hysteresis): ", rel) &&
            readInt("  Min duration samples (>= 1): ", minDur)) {
            const ThresholdEvent::CrossingType ct =
                (crossChoice == 2) ? ThresholdEvent::CrossingType::LOWER
                                   : ThresholdEvent::CrossingType::UPPER;
            eng.addDetector(ThresholdDetector(name, act, rel, minDur, ct, "processed"));
            std::cout << "  [OK] Added ThresholdDetector.\n";
        }
    } else if (sub == 3) {
        std::string name;
        int w = 32, s = 16;
        double zThr = 2.5, eThr = 2.5;
        if (readLine("  Detector name: ", name) &&
            readInt("  Window size W (>= 1): ", w) &&
            readInt("  Stride S (>= 1): ", s) &&
            readDouble("  Z-score threshold (> 0): ", zThr) &&
            readDouble("  Energy ratio threshold (> 0): ", eThr)) {
            eng.addDetector(WindowAnomalyDetector(name, w, s, zThr, eThr, "processed"));
            std::cout << "  [OK] Added WindowAnomalyDetector.\n";
        }
    } else if (sub == 4) {
        eng.clearDetectors();
        std::cout << "  [OK] Detectors cleared.\n";
    }
}

void handleRunAndTriage(StudySession& session) {
    std::cout << "\n  --- Analysis & Unit 7 Event Triage ---\n";
    std::cout << "  1. Run DetectionEngine across all channels (triage by Severity)\n";
    std::cout << "  2. Re-sort current events via EventSorter + Polymorphic Functor\n";
    std::cout << "  0. Back\n";

    int sub = 0;
    if (!readInt("  Select option: ", sub) || sub == 0) {
        return;
    }

    if (sub == 1) {
        const int found = session.runAnalysis(true);
        std::cout << "  [OK] Analysis complete: " << found
                  << " events detected and triaged via Quicksort.\n";
    } else if (sub == 2) {
        if (session.getRegistry().isEmpty()) {
            std::cout << "  [INFO] No events in registry to sort.\n";
            return;
        }
        int algo = 1, compChoice = 1, dir = 1;
        if (!readInt("  Algorithm (1=Quicksort O(n log n), 2=Insertion Sort O(n^2)): ", algo) ||
            !readInt("  Functor (1=Severity, 2=Chronological, 3=Duration, 4=Channel): ", compChoice) ||
            !readInt("  Direction (1=default/primary, 2=reversed): ", dir)) {
            return;
        }

        const bool useQuick = (algo != 2);
        const bool primary = (dir != 2);

        EventComparator* comparator = nullptr;
        if (compChoice == 1) {
            comparator = new SeverityComparator(primary);
        } else if (compChoice == 2) {
            comparator = new ChronologicalComparator(primary);
        } else if (compChoice == 3) {
            comparator = new DurationComparator(primary);
        } else {
            comparator = new ChannelComparator(primary);
        }

        try {
            session.sortEvents(*comparator, useQuick);
            std::cout << "  [OK] Sorted " << session.getRegistry().getCount() << " events using "
                      << (useQuick ? "Quicksort" : "Insertion Sort")
                      << " with " << comparator->getName() << ".\n";
        } catch (...) {
            delete comparator;
            throw;
        }
        delete comparator;
    }
}

void handleReports(const StudySession& session) {
    std::cout << "\n  --- 80-Column Study Report ---\n";
    std::cout << "  1. Print to console\n";
    std::cout << "  2. Export to text file\n";
    std::cout << "  0. Back\n";

    int sub = 0;
    if (!readInt("  Select option: ", sub) || sub == 0) {
        return;
    }

    int maxEvents = 25;
    if (!readInt("  Max events to display (0 = all): ", maxEvents)) {
        return;
    }

    if (sub == 1) {
        PersistenceManager::writeReport(session, std::cout, maxEvents);
    } else if (sub == 2) {
        std::string path;
        if (readLine("  Output report path (e.g. reports/study_report.txt): ", path)) {
            PersistenceManager::exportReport(session, path, maxEvents);
            std::cout << "  [OK] Report exported to \"" << path << "\".\n";
        }
    }
}

void handlePersistence(StudySession& session) {
    std::cout << "\n  --- StudySession Persistence ---\n";
    std::cout << "  1. Save current StudySession to disk\n";
    std::cout << "  2. Load StudySession from disk\n";
    std::cout << "  0. Back\n";

    int sub = 0;
    if (!readInt("  Select option: ", sub) || sub == 0) {
        return;
    }

    std::string path;
    if (!readLine("  Session file path (e.g. reports/session.dat): ", path)) {
        return;
    }

    if (sub == 1) {
        PersistenceManager::saveSession(session, path);
        std::cout << "  [OK] Session saved to \"" << path << "\".\n";
    } else if (sub == 2) {
        PersistenceManager::loadSession(path, session);
        std::cout << "  [OK] Session reconstructed: " << session << "\n";
    }
}

void handleBenchmarksMenu() {
    std::cout << "\n  --- Scientific Benchmarks (Real Biomedical Datasets) ---\n";
    std::cout << "  1. Run ALL available benchmarks (MIT-BIH + UCI + Bonn)\n";
    std::cout << "  2. MIT-BIH Arrhythmia Database (Record 100 QRS Validation)\n";
    std::cout << "  3. UCI EEG Eye State (14-Channel Impulse Suppression)\n";
    std::cout << "  4. Bonn University EEG (Healthy vs. Inter-ictal vs. Ictal)\n";
    std::cout << "  0. Back\n";

    int sub = 0;
    if (!readInt("  Select option: ", sub) || sub == 0) {
        return;
    }

    if (sub == 1) {
        const int n = BenchmarkSuite::runAllDefaultBenchmarks(std::cout, "");
        std::cout << "\n  [DONE] Completed " << n << " scientific benchmark(s).\n";
    } else if (sub == 2) {
        BenchmarkSuite::runMitBihBenchmark(
            "data/mitbih_100_10s.csv", "data/mitbih_100_10s_annotations.csv", std::cout);
    } else if (sub == 3) {
        std::string path;
        if (readLine("  UCI CSV path (Enter for data/uci_eeg_eyestate_2000.csv): ", path)) {
            if (path.empty()) {
                path = "data/uci_eeg_eyestate_2000.csv";
            }
            BenchmarkSuite::runUciEyeStateBenchmark(path, std::cout);
        }
    } else if (sub == 4) {
        std::string zPath, fPath, sPath;
        if (readLine("  Healthy Set A path (Enter for data/Z001.txt): ", zPath) &&
            readLine("  Inter-ictal Set D path (Enter for data/F001.txt): ", fPath) &&
            readLine("  Ictal Set E path (Enter for data/S001.txt): ", sPath)) {
            if (zPath.empty()) zPath = "data/Z001.txt";
            if (fPath.empty()) fPath = "data/F001.txt";
            if (sPath.empty()) sPath = "data/S001.txt";
            BenchmarkSuite::runBonnEegBenchmark(zPath, fPath, sPath, std::cout);
        }
    }
}

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [option]\n"
              << "  (no arguments)     Launch interactive console control panel\n"
              << "  --benchmark [dir]  Run all available real-dataset benchmarks (optionally exporting reports to dir)\n"
              << "  --benchmark-mit    Run MIT-BIH Record 100 QRS benchmark\n"
              << "  --benchmark-uci    Run UCI EEG Eye State 14-channel benchmark\n"
              << "  --benchmark-bonn   Run Bonn University EEG benchmark\n"
              << "  --help             Show this help message\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc >= 2) {
            const std::string arg(argv[1]);
            if (arg == "--help" || arg == "-h") {
                printUsage(argv[0]);
                return 0;
            }
            if (arg == "--benchmark") {
                const std::string reportDir = (argc >= 3) ? std::string(argv[2]) : "";
                const int ran = BenchmarkSuite::runAllDefaultBenchmarks(std::cout, reportDir);
                std::cout << "\n[SUMMARY] Benchmarks executed: " << ran << "/3\n";
                return (ran > 0) ? 0 : 1;
            }
            if (arg == "--benchmark-mit") {
                const std::string sigPath =
                    (argc >= 3) ? std::string(argv[2]) : "data/mitbih_100_10s.csv";
                const std::string annPath =
                    (argc >= 4) ? std::string(argv[3]) : "data/mitbih_100_10s_annotations.csv";
                const MitBihBenchmarkResult r =
                    BenchmarkSuite::runMitBihBenchmark(sigPath, annPath, std::cout);
                return r.executed ? 0 : 1;
            }
            if (arg == "--benchmark-uci") {
                const std::string path = (argc >= 3) ? std::string(argv[2])
                                                     : "data/uci_eeg_eyestate_2000.csv";
                const UciBenchmarkResult r = BenchmarkSuite::runUciEyeStateBenchmark(path, std::cout);
                return r.executed ? 0 : 1;
            }
            if (arg == "--benchmark-bonn") {
                const std::string zPath = (argc >= 3) ? std::string(argv[2]) : "data/Z001.txt";
                const std::string fPath = (argc >= 4) ? std::string(argv[3]) : "data/F001.txt";
                const std::string sPath = (argc >= 5) ? std::string(argv[4]) : "data/S001.txt";
                const BonnBenchmarkResult r =
                    BenchmarkSuite::runBonnEegBenchmark(zPath, fPath, sPath, std::cout);
                return r.executed ? 0 : 1;
            }

            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }

        StudySession session("InteractiveStudy_2026", 4);
        bool running = true;

        while (running) {
            printMainMenu(session);
            int choice = -1;
            if (!readInt("  Select option: ", choice)) {
                if (std::cin.eof()) {
                    break;
                }
                continue;
            }

            try {
                switch (choice) {
                    case 1:
                        handleLoadDataset(session);
                        break;
                    case 2:
                        handleConfigurePipeline(session);
                        break;
                    case 3:
                        handleApplyPipeline(session);
                        break;
                    case 4:
                        handleConfigureDetectors(session);
                        break;
                    case 5:
                        handleRunAndTriage(session);
                        break;
                    case 6:
                        handleReports(session);
                        break;
                    case 7:
                        handlePersistence(session);
                        break;
                    case 8:
                        handleBenchmarksMenu();
                        break;
                    case 9: {
                        std::string newName;
                        if (readLine("  New session name (Enter to keep current): ", newName)) {
                            if (!newName.empty()) {
                                session.setSessionName(newName);
                            }
                            session.clearAll();
                            std::cout << "  [OK] Session reset: " << session << "\n";
                        }
                        break;
                    }
                    case 0:
                        running = false;
                        break;
                    default:
                        std::cout << "  [WARN] Option out of range (0..9).\n";
                        break;
                }
            } catch (const std::exception& ex) {
                std::cout << "  [ERROR] " << ex.what() << "\n";
            }
        }

        std::cout << "\nExiting Time-Series Processing Engine cleanly.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 1;
    }
}
