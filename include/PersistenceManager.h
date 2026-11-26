#ifndef PERSISTENCE_MANAGER_H
#define PERSISTENCE_MANAGER_H

#include "ChannelSignal.h"

#include <iosfwd>
#include <string>

class DetectedEvent;
class EventDetector;
class ProcessingModule;
class StudySession;

/**
 * @struct IngestionStats
 * @brief Summary metrics reported after parsing an external scientific dataset file.
 */
struct IngestionStats {
    int totalLinesRead;
    int loadedSamples;
    int skippedLines;

    IngestionStats() : totalLinesRead(0), loadedSamples(0), skippedLines(0) {}
};

/**
 * @class PersistenceManager
 * @brief Non-instantiable Unit 6 I/O and persistence utility operating on C++ file/string streams
 *        without STL containers. Provides:
 *        1. Fault-tolerant ingestion of single-column TXT (Bonn EEG) and single/multi-column CSV
 *           (MIT-BIH ECG, UCI EEG Eye State) recovering cleanly from failbit, CR/LF line endings,
 *           empty lines, and corrupted tokens.
 *        2. Lossless serialization and polymorphic reconstruction of an entire StudySession
 *           (channels with raw & processed buffers, ProcessingPipeline stages, DetectionEngine
 *           strategies, and heterogeneous EventRegistry records).
 *        3. Aligned 80-column formatted clinical/technical reporting using stream manipulators.
 */
class PersistenceManager {
private:
    static constexpr int kInitialSampleCapacity = 256;
    static constexpr int kReportWidth = 80;

    static std::string trim(const std::string& text);
    static bool parseStrictDouble(const std::string& token, double& outValue);
    static bool parseStrictInt(const std::string& token, int& outValue);
    static bool extractDelimitedField(const std::string& line,
                                      int targetColumn,
                                      char delimiter,
                                      std::string& outField);
    static bool extractKeyValue(const std::string& serialized,
                                const std::string& key,
                                std::string& outValue);
    static std::string truncateField(const std::string& text, int maxWidth);

    static double* growDoubleBuffer(double* oldBuffer, int count, int newCapacity);

    static ProcessingModule* deserializeModule(const std::string& line);
    static std::string serializeDetector(const EventDetector& detector);
    static EventDetector* deserializeDetector(const std::string& line);
    static std::string serializeEvent(const DetectedEvent& event);
    static DetectedEvent* deserializeEvent(const std::string& line);

    static bool readDataLine(std::istream& in, std::string& outLine);

public:
    PersistenceManager() = delete;

    /**
     * @brief Loads a single-column numeric text file (e.g., Bonn EEG Z001.txt / S001.txt),
     *        recovering from failbit on corrupted or blank lines.
     * @throws std::runtime_error if the file cannot be opened or fewer than 2 valid samples exist.
     * @throws std::invalid_argument if maxSamples < 0.
     */
    static ChannelSignal loadSingleColumnTxt(const std::string& filepath,
                                             const std::string& channelCode,
                                             const std::string& unit,
                                             double samplingRate,
                                             int maxSamples = 0,
                                             IngestionStats* outStats = nullptr);

    /**
     * @brief Loads a single numeric column from a CSV file (e.g., MIT-BIH MLII_mV or UCI AF4),
     *        recovering cleanly from failbit and malformed rows.
     * @throws std::invalid_argument if columnIndex < 0 or maxSamples < 0.
     * @throws std::runtime_error if the file cannot be opened or fewer than 2 valid samples exist.
     */
    static ChannelSignal loadCsvColumn(const std::string& filepath,
                                       int columnIndex,
                                       bool hasHeader,
                                       const std::string& channelCode,
                                       const std::string& unit,
                                       double samplingRate,
                                       char delimiter = ',',
                                       int maxSamples = 0,
                                       IngestionStats* outStats = nullptr);

    /**
     * @brief Loads multiple synchronized columns from a CSV file in a single stream pass
     *        and appends each resulting ChannelSignal to session. Rows with any corrupted
     *        selected column are skipped atomically so all channels remain time-aligned.
     * @throws std::invalid_argument on invalid array arguments, negative column index, or maxSamples < 0.
     * @throws std::runtime_error if the file cannot be opened or fewer than 2 valid rows exist.
     */
    static int loadMultiChannelCsv(const std::string& filepath,
                                   StudySession& session,
                                   const std::string* channelCodes,
                                   const int* columnIndices,
                                   int channelCount,
                                   const std::string& unit,
                                   double samplingRate,
                                   bool hasHeader,
                                   char delimiter = ',',
                                   int maxSamples = 0,
                                   IngestionStats* outStats = nullptr);

    /**
     * @brief Loads beat annotation sample indices from a MIT-BIH CSV annotation file
     *        (e.g., data/mitbih_100_10s_annotations.csv), filtering out non-beat rhythm markers
     *        such as '+' so ground-truth QRS counts match real heartbeats.
     * @param outIndices Output pointer receiving a newly heap-allocated int array (caller must delete[] it).
     * @param outCount Output reference receiving the number of beat annotations loaded.
     * @return true on success, false if file could not be opened.
     */
    static bool loadMitBihBeatAnnotations(const std::string& filepath,
                                          int*& outIndices,
                                          int& outCount,
                                          bool hasHeader = true);

    /**
     * @brief Serializes the full state of session (metadata, pipeline stages, detectors,
     *        channels with raw + processed buffers, and all detected events) to a plain-text file.
     * @throws std::runtime_error if the file cannot be opened or a write error occurs.
     */
    static void saveSession(const StudySession& session, const std::string& filepath);

    /**
     * @brief Reconstructs a complete StudySession polymorphically from a file saved by saveSession().
     *        Provides strong safety: destination is only overwritten if the entire file parses validly.
     * @throws std::runtime_error if the file cannot be opened, header is invalid, or data is corrupt/truncated.
     */
    static void loadSession(const std::string& filepath, StudySession& outSession);

    /**
     * @brief Writes an aligned 80-column formatted analytical report of session to the output stream
     *        using standard C++ I/O manipulators (setw, fixed, setprecision, setfill).
     */
    static void writeReport(const StudySession& session,
                            std::ostream& os,
                            int maxEventsToDisplay = 0);

    /**
     * @brief Exports the aligned 80-column analytical report directly to a text file.
     * @throws std::runtime_error if the file cannot be opened or writing fails.
     */
    static void exportReport(const StudySession& session,
                             const std::string& filepath,
                             int maxEventsToDisplay = 0);
};

#endif
