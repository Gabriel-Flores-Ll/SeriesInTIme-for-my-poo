#include "ChannelComparator.h"
#include "ChannelSignal.h"
#include "ChannelStadistics.h"
#include "DetectecEvent.h"
#include "DifferentialFilter.h"
#include "DurationComparator.h"
#include "MedianFilter.h"
#include "MovingAverageFilter.h"
#include "PeakDetector.h"
#include "PeakEvent.h"
#include "PersistenceManager.h"
#include "ProcessingPipeline.h"
#include "SeverityComparator.h"
#include "StatisticalAnomalyEvent.h"
#include "StudySession.h"
#include "ThresholdDetector.h"
#include "ThresholdEvent.h"
#include "ThresholdLimiter.h"
#include "WaveRectifier.h"
#include "WindowAnomalyDetector.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
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
void expectRuntimeError(Fn fn, const char* label) {
    try {
        fn();
        ++gFailed;
        std::cerr << "[FAIL] Expected std::runtime_error: " << label << "\n";
    } catch (const std::runtime_error&) {
        ++gPassed;
    } catch (...) {
        ++gFailed;
        std::cerr << "[FAIL] Wrong exception (expected runtime_error): " << label << "\n";
    }
}

bool fileExists(const std::string& path) {
    std::ifstream in(path);
    return in.good();
}

void testFaultTolerantIngestion() {
    const std::string corruptTxt = "/tmp/test_fase5_corrupt.txt";
    {
        std::ofstream out(corruptTxt);
        out << "12.5\r\n"
            << "CORRUPT_SENSOR_LINE\r\n"
            << "\r\n"
            << "   25.0   \r\n"
            << "30.0abc\n"
            << "NaN\n"
            << "inf\n"
            << "-7.5\n"
            << "100.0\r\n";
    }

    IngestionStats txtStats;
    ChannelSignal txtSig = PersistenceManager::loadSingleColumnTxt(
        corruptTxt, "CORRUPT_TXT", "uV", 173.61, 0, &txtStats);

    check(txtStats.loadedSamples == 4 && txtStats.skippedLines == 5 && txtStats.totalLinesRead == 9,
          "loadSingleColumnTxt recovers from failbit, CR/LF, NaN/Inf, trailing garbage, and blank lines");
    check(txtSig.getNumberOfSamples() == 4 &&
          approxEqual(txtSig.getRawElement(0), 12.5) &&
          approxEqual(txtSig.getRawElement(1), 25.0) &&
          approxEqual(txtSig.getRawElement(2), -7.5) &&
          approxEqual(txtSig.getRawElement(3), 100.0),
          "loadSingleColumnTxt stores exact valid finite samples");

    const std::string corruptCsv = "/tmp/test_fase5_corrupt.csv";
    {
        std::ofstream out(corruptCsv);
        out << "idx,CH_A,CH_B\r\n"
            << "0,1.5,10.0\r\n"
            << "1,BAD_VAL,20.0\r\n"
            << "2,3.5\r\n"
            << "3,4.5,40.0\r\n"
            << "4,5.5,50.0\r\n";
    }

    StudySession csvSess("CorruptCSVTest", 2);
    const std::string codes[2] = {"CH_A", "CH_B"};
    const int cols[2] = {1, 2};
    IngestionStats csvStats;
    const int loadedCh = PersistenceManager::loadMultiChannelCsv(
        corruptCsv, csvSess, codes, cols, 2, "mV", 360.0, true, ',', 0, &csvStats);

    check(loadedCh == 2 && csvSess.getChannelCount() == 2,
          "loadMultiChannelCsv adds both synchronized channels");
    check(csvStats.loadedSamples == 3 && csvStats.skippedLines == 2,
          "loadMultiChannelCsv skips corrupt and short rows atomically");
    check(approxEqual(csvSess.getChannel(0).getRawElement(1), 4.5) &&
          approxEqual(csvSess.getChannel(1).getRawElement(1), 40.0),
          "loadMultiChannelCsv keeps channels time-synchronized after skipped rows");

    expectRuntimeError([]() {
        (void)PersistenceManager::loadSingleColumnTxt("/tmp/nonexistent_file_fase5.txt", "X", "uV", 100.0);
    }, "loadSingleColumnTxt throws runtime_error on missing file");

    std::remove(corruptTxt.c_str());
    std::remove(corruptCsv.c_str());
}

void testStudySessionRuleOfThreeAndRoundTripPersistence() {
    StudySession original("ClinicalRoundTrip_2026", 2);

    double eegRaw[30];
    double ecgRaw[30];
    for (int i = 0; i < 30; ++i) {
        eegRaw[i] = (i % 2 == 0) ? 10.5 : 9.5;
        ecgRaw[i] = (i % 2 == 0) ? 0.1 : -0.1;
    }
    eegRaw[5] = 850.0;
    for (int i = 16; i <= 22; ++i) {
        eegRaw[i] = 55.0;
    }
    ecgRaw[10] = 7.5;
    ecgRaw[11] = 7.5;
    ecgRaw[24] = 6.0;
    ecgRaw[25] = 6.0;

    original.addChannel(ChannelSignal("EEG_AF4", "uV", 128.0, eegRaw, 30));
    original.addChannel(ChannelSignal("ECG_MLII", "mV", 360.0, ecgRaw, 30));

    original.getPipeline().addStage(MedianFilter(3));
    MovingAverageFilter bypassedMa(5);
    bypassedMa.setActive(false);
    original.getPipeline().addStage(bypassedMa);
    DifferentialFilter bypassedTkeo;
    bypassedTkeo.setActive(false);
    original.getPipeline().addStage(bypassedTkeo);
    WaveRectifier bypassedRect(RectifierMode::HalfWave, 1.5);
    bypassedRect.setActive(false);
    original.getPipeline().addStage(bypassedRect);
    ThresholdLimiter lim(-100.0, 100.0, 1.0);
    original.getPipeline().addStage(lim);

    check(original.applyPipelineToAll("raw"), "StudySession::applyPipelineToAll(\"raw\") succeeds");
    check(original.getChannel(0).getProcessedElement(5) < 15.0 &&
          approxEqual(original.getChannel(0).getRawElement(5), 850.0),
          "Pipeline cleans EEG spike while preserving immutable raw buffer");

    original.getEngine().addDetector(PeakDetector("r_peaks", 4.0, 2.0, 5, true, "processed"));
    original.getEngine().addDetector(
        ThresholdDetector("burst_thr", 30.0, 20.0, 3, ThresholdEvent::CrossingType::UPPER, "processed"));
    original.getEngine().addDetector(
        WindowAnomalyDetector("win_anom", 4, 2, 1.5, 2.5, "processed"));

    const int detectedCount = original.runAnalysis(true);
    check(detectedCount >= 4 &&
          original.getRegistry().countByType("PEAK") >= 2 &&
          original.getRegistry().countByType("THRESHOLD") >= 1 &&
          original.getRegistry().countByType("STATISTICAL_ANOMALY") >= 1,
          "StudySession::runAnalysis populates all 3 concrete event types");

    StudySession deepCopy(original);
    check(deepCopy.getChannelCount() == 2 &&
          &deepCopy.getChannel(0) != &original.getChannel(0) &&
          deepCopy.getPipeline() == original.getPipeline() &&
          deepCopy.getRegistry() == original.getRegistry(),
          "StudySession copy constructor performs deep copy across all subsystems");

    StudySession assigned("Temp", 1);
    assigned = original;
    StudySession& selfRef = assigned;
    assigned = selfRef;
    check(assigned.getSessionName() == original.getSessionName() &&
          assigned.getRegistry() == original.getRegistry(),
          "StudySession assignment and self-assignment preserve state");

    const std::string sessionFile = "/tmp/test_fase5_session.dat";
    PersistenceManager::saveSession(original, sessionFile);

    StudySession reloaded("Placeholder", 0);
    PersistenceManager::loadSession(sessionFile, reloaded);

    check(reloaded.getSessionName() == original.getSessionName(),
          "Round-trip preserves sessionName");
    check(reloaded.getPipeline() == original.getPipeline() &&
          reloaded.getPipeline().getCount() == 5 &&
          !reloaded.getPipeline()[1].isActive() &&
          reloaded.getPipeline()[0].isActive(),
          "Round-trip reconstructs all 5 polymorphic ProcessingModule stages and active flags");
    check(reloaded.getEngine().getDetectorCount() == 3 &&
          reloaded.getEngine().getDetector(0).getDescription() ==
              original.getEngine().getDetector(0).getDescription() &&
          reloaded.getEngine().getDetector(1).getDescription() ==
              original.getEngine().getDetector(1).getDescription() &&
          reloaded.getEngine().getDetector(2).getDescription() ==
              original.getEngine().getDetector(2).getDescription(),
          "Round-trip reconstructs all 3 polymorphic EventDetector strategies identically");
    check(reloaded.getChannelCount() == 2 &&
          reloaded.findChannelByCode("EEG_AF4") == 0 &&
          reloaded.findChannelByCode("ECG_MLII") == 1 &&
          approxEqual(reloaded.getChannel(0).getRawElement(5), 850.0) &&
          approxEqual(reloaded.getChannel(0).getProcessedElement(5),
                      original.getChannel(0).getProcessedElement(5)),
          "Round-trip reconstructs ChannelSignals with both raw and processed buffers intact");
    check(reloaded.getRegistry() == original.getRegistry(),
          "Round-trip reconstructs heterogeneous EventRegistry losslessly");

    const std::string badSession = "/tmp/test_fase5_bad_session.dat";
    {
        std::ofstream out(badSession);
        out << "[STUDY_SESSION_V1]\nNAME;Broken\n[PIPELINE];1\nUNKNOWN_FILTER;W=5\n";
    }
    expectRuntimeError([&badSession, &reloaded]() {
        PersistenceManager::loadSession(badSession, reloaded);
    }, "loadSession throws runtime_error on unknown/corrupt module");
    check(reloaded.getSessionName() == original.getSessionName() &&
          reloaded.getChannelCount() == 2,
          "loadSession preserves destination StudySession unchanged on error");

    std::ostringstream reportStream;
    PersistenceManager::writeReport(reloaded, reportStream, 10);
    const std::string reportText = reportStream.str();
    check(reportText.find("TIME-SERIES STUDY REPORT") != std::string::npos &&
          reportText.find("EEG_AF4") != std::string::npos &&
          reportText.find("ECG_MLII") != std::string::npos,
          "writeReport formats complete session summary, channels, pipeline, and events");

    bool allLinesWithin80Cols = true;
    std::istringstream lineReader(reportText);
    std::string rline;
    while (std::getline(lineReader, rline)) {
        if (rline.size() > 80) {
            allLinesWithin80Cols = false;
            break;
        }
    }
    check(allLinesWithin80Cols, "Every line of writeReport fits strictly within 80 columns");

    const std::string reportFile = "/tmp/test_fase5_report.txt";
    PersistenceManager::exportReport(reloaded, reportFile, 10);
    check(fileExists(reportFile), "exportReport writes report file cleanly");

    std::remove(sessionFile.c_str());
    std::remove(badSession.c_str());
    std::remove(reportFile.c_str());
}

void testRealBiomedicalDatasetsIfPresent() {
    if (fileExists("data/uci_eeg_eyestate_2000.csv")) {
        IngestionStats uciStats;
        ChannelSignal af4 = PersistenceManager::loadCsvColumn(
            "data/uci_eeg_eyestate_2000.csv", 13, false, "UCI_AF4", "uV", 128.0, ',', 2000, &uciStats);
        check(af4.getNumberOfSamples() == 2000 && uciStats.skippedLines == 0,
              "Real dataset: loads 2000 samples of UCI EEG channel AF4 cleanly");
        check(approxEqual(af4.getRawElement(898), 715897.0, 1e-2),
              "Real dataset: verifies UCI AF4 impulsive spike 715897.0 at index 898");

        MedianFilter med5(5);
        check(af4.applyModule(med5, "raw") && af4.getProcessedElement(898) < 5000.0,
              "Real dataset: MedianFilter(5) eliminates UCI AF4 715897.0 artifact completely");
    }

    if (fileExists("data/mitbih_100_10s.csv") && fileExists("data/mitbih_100_10s_annotations.csv")) {
        StudySession mit("MIT_BIH_100", 2);
        const std::string mitCodes[2] = {"MLII", "V5"};
        const int mitCols[2] = {1, 2};
        IngestionStats mitStats;
        const int nCh = PersistenceManager::loadMultiChannelCsv(
            "data/mitbih_100_10s.csv", mit, mitCodes, mitCols, 2, "mV", 360.0, true, ',', 0, &mitStats);
        check(nCh == 2 && mitStats.loadedSamples == 3600 && mitStats.skippedLines == 0,
              "Real dataset: loads 3600 synchronized samples of MIT-BIH Record 100 (MLII + V5)");

        int* beatIndices = nullptr;
        int beatCount = 0;
        check(PersistenceManager::loadMitBihBeatAnnotations(
                  "data/mitbih_100_10s_annotations.csv", beatIndices, beatCount, true) &&
                  beatCount == 13 && beatIndices != nullptr && *beatIndices == 77,
              "Real dataset: loads MIT-BIH beat annotations filtering '+' rhythm marker (13 beats, first at 77)");
        delete[] beatIndices;
    }

    if (fileExists("data/Z001.txt")) {
        IngestionStats bonnStats;
        ChannelSignal bonnZ = PersistenceManager::loadSingleColumnTxt(
            "data/Z001.txt", "BONN_Z001", "uV", 173.61, 0, &bonnStats);
        check(bonnZ.getNumberOfSamples() == 4097 && bonnStats.skippedLines == 0,
              "Real dataset: loads all 4097 samples of Bonn EEG Z001.txt");
    }
}

}  // namespace

int main() {
    testFaultTolerantIngestion();
    testStudySessionRuleOfThreeAndRoundTripPersistence();
    testRealBiomedicalDatasetsIfPresent();

    std::cout << "Phase 5 Streams, StudySession & Persistence Tests -> Passed: " << gPassed
              << " | Failed: " << gFailed << "\n";
    return (gFailed == 0) ? 0 : 1;
}
