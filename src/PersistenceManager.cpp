#include "PersistenceManager.h"

#include "ChannelStadistics.h"
#include "DetectecEvent.h"
#include "DetectionEngine.h"
#include "DifferentialFilter.h"
#include "EventDetector.h"
#include "EventRegistry.h"
#include "MedianFilter.h"
#include "MetadataSensor.h"
#include "MovingAverageFilter.h"
#include "PeakDetector.h"
#include "PeakEvent.h"
#include "ProcessingModule.h"
#include "ProcessingPipeline.h"
#include "StatisticalAnomalyEvent.h"
#include "StudySession.h"
#include "ThresholdDetector.h"
#include "ThresholdEvent.h"
#include "ThresholdLimiter.h"
#include "WaveRectifier.h"
#include "WindowAnomalyDetector.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

std::string PersistenceManager::trim(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

bool PersistenceManager::parseStrictDouble(const std::string& token, double& outValue) {
    const std::string cleaned = trim(token);
    if (cleaned.empty()) {
        return false;
    }

    std::istringstream iss(cleaned);
    double parsed = 0.0;
    iss >> parsed;
    if (iss.fail()) {
        iss.clear();
        return false;
    }

    char trailing = '\0';
    if (iss >> trailing) {
        return false;
    }

    if (!std::isfinite(parsed)) {
        return false;
    }

    outValue = parsed;
    return true;
}

bool PersistenceManager::parseStrictInt(const std::string& token, int& outValue) {
    const std::string cleaned = trim(token);
    if (cleaned.empty()) {
        return false;
    }

    std::istringstream iss(cleaned);
    int parsed = 0;
    iss >> parsed;
    if (iss.fail()) {
        iss.clear();
        return false;
    }

    char trailing = '\0';
    if (iss >> trailing) {
        return false;
    }

    outValue = parsed;
    return true;
}

bool PersistenceManager::extractDelimitedField(const std::string& line,
                                               int targetColumn,
                                               char delimiter,
                                               std::string& outField) {
    if (targetColumn < 0) {
        return false;
    }

    int currentCol = 0;
    std::size_t fieldStart = 0;

    for (std::size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == delimiter) {
            if (currentCol == targetColumn) {
                outField = line.substr(fieldStart, i - fieldStart);
                return true;
            }
            ++currentCol;
            fieldStart = i + 1;
        }
    }
    return false;
}

bool PersistenceManager::extractKeyValue(const std::string& serialized,
                                         const std::string& key,
                                         std::string& outValue) {
    const std::string prefix = key + "=";
    std::size_t pos = 0;

    while (pos <= serialized.size()) {
        std::size_t sep = serialized.find(';', pos);
        if (sep == std::string::npos) {
            sep = serialized.size();
        }

        const std::string token = trim(serialized.substr(pos, sep - pos));
        if (token.size() >= prefix.size() && token.compare(0, prefix.size(), prefix) == 0) {
            outValue = token.substr(prefix.size());
            return true;
        }

        if (sep == serialized.size()) {
            break;
        }
        pos = sep + 1;
    }

    return false;
}

std::string PersistenceManager::truncateField(const std::string& text, int maxWidth) {
    if (maxWidth <= 0) {
        return "";
    }
    if (static_cast<int>(text.size()) <= maxWidth) {
        return text;
    }
    if (maxWidth <= 2) {
        return text.substr(0, static_cast<std::size_t>(maxWidth));
    }
    return text.substr(0, static_cast<std::size_t>(maxWidth - 2)) + "..";
}

double* PersistenceManager::growDoubleBuffer(double* oldBuffer, int count, int newCapacity) {
    double* newBuffer = new double[newCapacity];
    for (int i = 0; i < count; ++i) {
        *(newBuffer + i) = *(oldBuffer + i);
    }
    delete[] oldBuffer;
    return newBuffer;
}

bool PersistenceManager::readDataLine(std::istream& in, std::string& outLine) {
    while (std::getline(in, outLine)) {
        outLine = trim(outLine);
        if (!outLine.empty()) {
            return true;
        }
    }
    return false;
}

ProcessingModule* PersistenceManager::deserializeModule(const std::string& line) {
    const std::string cleaned = trim(line);
    std::string type;
    if (!extractDelimitedField(cleaned, 0, ';', type)) {
        throw std::runtime_error("PersistenceManager: empty module record.");
    }
    type = trim(type);

    std::string activeStr;
    int activeFlag = 1;
    if (extractKeyValue(cleaned, "active", activeStr)) {
        if (!parseStrictInt(activeStr, activeFlag) || (activeFlag != 0 && activeFlag != 1)) {
            throw std::runtime_error("PersistenceManager: invalid active flag in module record.");
        }
    }

    ProcessingModule* module = nullptr;

    if (type == "MOVING_AVERAGE") {
        std::string wStr;
        int w = 5;
        if (!extractKeyValue(cleaned, "W", wStr) || !parseStrictInt(wStr, w)) {
            throw std::runtime_error("PersistenceManager: malformed MOVING_AVERAGE record.");
        }
        module = new MovingAverageFilter(w);
    } else if (type == "MEDIAN") {
        std::string wStr;
        int w = 5;
        if (!extractKeyValue(cleaned, "W", wStr) || !parseStrictInt(wStr, w)) {
            throw std::runtime_error("PersistenceManager: malformed MEDIAN record.");
        }
        module = new MedianFilter(w);
    } else if (type == "DIFFERENTIAL_TKEO") {
        module = new DifferentialFilter();
    } else if (type == "RECTIFIER") {
        std::string modeStr;
        std::string scaleStr;
        double scale = 1.0;
        if (!extractKeyValue(cleaned, "mode", modeStr) ||
            !extractKeyValue(cleaned, "scale", scaleStr) ||
            !parseStrictDouble(scaleStr, scale)) {
            throw std::runtime_error("PersistenceManager: malformed RECTIFIER record.");
        }
        RectifierMode mode = RectifierMode::FullWave;
        if (modeStr == "FULL_WAVE") {
            mode = RectifierMode::FullWave;
        } else if (modeStr == "HALF_WAVE") {
            mode = RectifierMode::HalfWave;
        } else {
            throw std::runtime_error("PersistenceManager: unknown RECTIFIER mode.");
        }
        module = new WaveRectifier(mode, scale);
    } else if (type == "LIMITER") {
        std::string minStr;
        std::string maxStr;
        std::string scaleStr;
        double minVal = -1.0;
        double maxVal = 1.0;
        double scale = 1.0;
        if (!extractKeyValue(cleaned, "min", minStr) ||
            !extractKeyValue(cleaned, "max", maxStr) ||
            !extractKeyValue(cleaned, "scale", scaleStr) ||
            !parseStrictDouble(minStr, minVal) ||
            !parseStrictDouble(maxStr, maxVal) ||
            !parseStrictDouble(scaleStr, scale)) {
            throw std::runtime_error("PersistenceManager: malformed LIMITER record.");
        }
        module = new ThresholdLimiter(minVal, maxVal, scale);
    } else {
        throw std::runtime_error("PersistenceManager: unknown module type \"" + type + "\".");
    }

    module->setActive(activeFlag == 1);
    return module;
}

std::string PersistenceManager::serializeDetector(const EventDetector& detector) {
    std::ostringstream oss;
    oss << std::setprecision(10);

    if (const auto* pk = dynamic_cast<const PeakDetector*>(&detector)) {
        oss << "PEAK_DETECTOR"
            << ";name=" << pk->getName()
            << ";buf=" << pk->getBufferSelector()
            << ";minAmp=" << pk->getMinAmplitude()
            << ";minProm=" << pk->getMinProminence()
            << ";minDist=" << pk->getMinDistanceSamples()
            << ";valleys=" << (pk->isDetectingValleys() ? 1 : 0);
        return oss.str();
    }

    if (const auto* th = dynamic_cast<const ThresholdDetector*>(&detector)) {
        oss << "THRESHOLD_DETECTOR"
            << ";name=" << th->getName()
            << ";buf=" << th->getBufferSelector()
            << ";act=" << th->getActivationThreshold()
            << ";rel=" << th->getReleaseThreshold()
            << ";minDur=" << th->getMinDurationSamples()
            << ";cross="
            << (th->getCrossingType() == ThresholdEvent::CrossingType::UPPER ? "UPPER" : "LOWER");
        return oss.str();
    }

    if (const auto* wa = dynamic_cast<const WindowAnomalyDetector*>(&detector)) {
        oss << "WINDOW_ANOMALY_DETECTOR"
            << ";name=" << wa->getName()
            << ";buf=" << wa->getBufferSelector()
            << ";W=" << wa->getWindowSize()
            << ";S=" << wa->getStride()
            << ";zThr=" << wa->getZScoreThreshold()
            << ";eThr=" << wa->getEnergyRatioThreshold();
        return oss.str();
    }

    throw std::runtime_error("PersistenceManager: unsupported concrete EventDetector subtype.");
}

EventDetector* PersistenceManager::deserializeDetector(const std::string& line) {
    const std::string cleaned = trim(line);
    std::string type;
    if (!extractDelimitedField(cleaned, 0, ';', type)) {
        throw std::runtime_error("PersistenceManager: empty detector record.");
    }
    type = trim(type);

    std::string name;
    std::string buf;
    if (!extractKeyValue(cleaned, "name", name) || !extractKeyValue(cleaned, "buf", buf)) {
        throw std::runtime_error("PersistenceManager: missing detector base fields.");
    }

    if (type == "PEAK_DETECTOR") {
        std::string ampS, promS, distS, valS;
        double amp = 0.0, prom = 0.0;
        int dist = 1, valFlag = 0;
        if (!extractKeyValue(cleaned, "minAmp", ampS) ||
            !extractKeyValue(cleaned, "minProm", promS) ||
            !extractKeyValue(cleaned, "minDist", distS) ||
            !extractKeyValue(cleaned, "valleys", valS) ||
            !parseStrictDouble(ampS, amp) ||
            !parseStrictDouble(promS, prom) ||
            !parseStrictInt(distS, dist) ||
            !parseStrictInt(valS, valFlag) ||
            (valFlag != 0 && valFlag != 1)) {
            throw std::runtime_error("PersistenceManager: malformed PEAK_DETECTOR record.");
        }
        return new PeakDetector(name, amp, prom, dist, valFlag == 1, buf);
    }

    if (type == "THRESHOLD_DETECTOR") {
        std::string actS, relS, durS, crossS;
        double act = 0.0, rel = 0.0;
        int minDur = 1;
        if (!extractKeyValue(cleaned, "act", actS) ||
            !extractKeyValue(cleaned, "rel", relS) ||
            !extractKeyValue(cleaned, "minDur", durS) ||
            !extractKeyValue(cleaned, "cross", crossS) ||
            !parseStrictDouble(actS, act) ||
            !parseStrictDouble(relS, rel) ||
            !parseStrictInt(durS, minDur)) {
            throw std::runtime_error("PersistenceManager: malformed THRESHOLD_DETECTOR record.");
        }
        ThresholdEvent::CrossingType ct = ThresholdEvent::CrossingType::UPPER;
        if (crossS == "UPPER") {
            ct = ThresholdEvent::CrossingType::UPPER;
        } else if (crossS == "LOWER") {
            ct = ThresholdEvent::CrossingType::LOWER;
        } else {
            throw std::runtime_error("PersistenceManager: invalid crossing type in THRESHOLD_DETECTOR.");
        }
        return new ThresholdDetector(name, act, rel, minDur, ct, buf);
    }

    if (type == "WINDOW_ANOMALY_DETECTOR") {
        std::string wS, sS, zS, eS;
        int w = 1, s = 1;
        double zThr = 2.5, eThr = 2.5;
        if (!extractKeyValue(cleaned, "W", wS) ||
            !extractKeyValue(cleaned, "S", sS) ||
            !extractKeyValue(cleaned, "zThr", zS) ||
            !extractKeyValue(cleaned, "eThr", eS) ||
            !parseStrictInt(wS, w) ||
            !parseStrictInt(sS, s) ||
            !parseStrictDouble(zS, zThr) ||
            !parseStrictDouble(eS, eThr)) {
            throw std::runtime_error("PersistenceManager: malformed WINDOW_ANOMALY_DETECTOR record.");
        }
        return new WindowAnomalyDetector(name, w, s, zThr, eThr, buf);
    }

    throw std::runtime_error("PersistenceManager: unknown detector type \"" + type + "\".");
}

std::string PersistenceManager::serializeEvent(const DetectedEvent& event) {
    std::ostringstream oss;
    oss << std::setprecision(10);

    if (const auto* pk = dynamic_cast<const PeakEvent*>(&event)) {
        oss << "PEAK"
            << ";ch=" << pk->getCodeOfChannel()
            << ";unit=" << pk->getUnitOfMeasure()
            << ";idx=" << pk->getInitIndex()
            << ";t=" << pk->getStartTimeInSecs()
            << ";sev=" << pk->getSeverity()
            << ";amp=" << pk->getPeakAmplitude()
            << ";prom=" << pk->getProminence()
            << ";pos=" << (pk->isPositivePeak() ? 1 : 0);
        return oss.str();
    }

    if (const auto* th = dynamic_cast<const ThresholdEvent*>(&event)) {
        oss << "THRESHOLD"
            << ";ch=" << th->getCodeOfChannel()
            << ";unit=" << th->getUnitOfMeasure()
            << ";i0=" << th->getInitIndex()
            << ";i1=" << th->getEndIndex()
            << ";t0=" << th->getStartTimeInSecs()
            << ";t1=" << th->getEndTimeInSecs()
            << ";sev=" << th->getSeverity()
            << ";thr=" << th->getReferenceThreshold()
            << ";ext=" << th->getExtremeValue()
            << ";cross="
            << (th->getCrossingType() == ThresholdEvent::CrossingType::UPPER ? "UPPER" : "LOWER");
        return oss.str();
    }

    if (const auto* an = dynamic_cast<const StatisticalAnomalyEvent*>(&event)) {
        oss << "STATISTICAL_ANOMALY"
            << ";ch=" << an->getCodeOfChannel()
            << ";unit=" << an->getUnitOfMeasure()
            << ";i0=" << an->getInitIndex()
            << ";i1=" << an->getEndIndex()
            << ";t0=" << an->getStartTimeInSecs()
            << ";t1=" << an->getEndTimeInSecs()
            << ";sev=" << an->getSeverity()
            << ";z=" << an->getMaxZScore()
            << ";er=" << an->getEnergyRatio()
            << ";mu=" << an->getBaselineMean()
            << ";sd=" << an->getBaselineStdDev()
            << ";wins=" << an->getInvolvedWindows();
        return oss.str();
    }

    throw std::runtime_error("PersistenceManager: unsupported concrete DetectedEvent subtype.");
}

DetectedEvent* PersistenceManager::deserializeEvent(const std::string& line) {
    const std::string cleaned = trim(line);
    std::string type;
    if (!extractDelimitedField(cleaned, 0, ';', type)) {
        throw std::runtime_error("PersistenceManager: empty event record.");
    }
    type = trim(type);

    std::string ch, unit, sevS;
    double sev = 0.0;
    if (!extractKeyValue(cleaned, "ch", ch) ||
        !extractKeyValue(cleaned, "unit", unit) ||
        !extractKeyValue(cleaned, "sev", sevS) ||
        !parseStrictDouble(sevS, sev)) {
        throw std::runtime_error("PersistenceManager: missing base fields in event record.");
    }

    if (type == "PEAK") {
        std::string idxS, tS, ampS, promS, posS;
        int idx = 0, posFlag = 1;
        double t = 0.0, amp = 0.0, prom = 0.0;
        if (!extractKeyValue(cleaned, "idx", idxS) ||
            !extractKeyValue(cleaned, "t", tS) ||
            !extractKeyValue(cleaned, "amp", ampS) ||
            !extractKeyValue(cleaned, "prom", promS) ||
            !extractKeyValue(cleaned, "pos", posS) ||
            !parseStrictInt(idxS, idx) ||
            !parseStrictDouble(tS, t) ||
            !parseStrictDouble(ampS, amp) ||
            !parseStrictDouble(promS, prom) ||
            !parseStrictInt(posS, posFlag) ||
            (posFlag != 0 && posFlag != 1)) {
            throw std::runtime_error("PersistenceManager: malformed PEAK event record.");
        }
        return new PeakEvent(ch, unit, idx, t, sev, amp, prom, posFlag == 1);
    }

    if (type == "THRESHOLD") {
        std::string i0S, i1S, t0S, t1S, thrS, extS, crossS;
        int i0 = 0, i1 = 0;
        double t0 = 0.0, t1 = 0.0, thr = 0.0, ext = 0.0;
        if (!extractKeyValue(cleaned, "i0", i0S) ||
            !extractKeyValue(cleaned, "i1", i1S) ||
            !extractKeyValue(cleaned, "t0", t0S) ||
            !extractKeyValue(cleaned, "t1", t1S) ||
            !extractKeyValue(cleaned, "thr", thrS) ||
            !extractKeyValue(cleaned, "ext", extS) ||
            !extractKeyValue(cleaned, "cross", crossS) ||
            !parseStrictInt(i0S, i0) ||
            !parseStrictInt(i1S, i1) ||
            !parseStrictDouble(t0S, t0) ||
            !parseStrictDouble(t1S, t1) ||
            !parseStrictDouble(thrS, thr) ||
            !parseStrictDouble(extS, ext)) {
            throw std::runtime_error("PersistenceManager: malformed THRESHOLD event record.");
        }
        ThresholdEvent::CrossingType ct = ThresholdEvent::CrossingType::UPPER;
        if (crossS == "UPPER") {
            ct = ThresholdEvent::CrossingType::UPPER;
        } else if (crossS == "LOWER") {
            ct = ThresholdEvent::CrossingType::LOWER;
        } else {
            throw std::runtime_error("PersistenceManager: invalid crossing type in THRESHOLD event.");
        }
        return new ThresholdEvent(ch, unit, i0, i1, t0, t1, sev, thr, ext, ct);
    }

    if (type == "STATISTICAL_ANOMALY") {
        std::string i0S, i1S, t0S, t1S, zS, erS, muS, sdS, winsS;
        int i0 = 0, i1 = 0, wins = 1;
        double t0 = 0.0, t1 = 0.0, z = 0.0, er = 0.0, mu = 0.0, sd = 0.0;
        if (!extractKeyValue(cleaned, "i0", i0S) ||
            !extractKeyValue(cleaned, "i1", i1S) ||
            !extractKeyValue(cleaned, "t0", t0S) ||
            !extractKeyValue(cleaned, "t1", t1S) ||
            !extractKeyValue(cleaned, "z", zS) ||
            !extractKeyValue(cleaned, "er", erS) ||
            !extractKeyValue(cleaned, "mu", muS) ||
            !extractKeyValue(cleaned, "sd", sdS) ||
            !extractKeyValue(cleaned, "wins", winsS) ||
            !parseStrictInt(i0S, i0) ||
            !parseStrictInt(i1S, i1) ||
            !parseStrictDouble(t0S, t0) ||
            !parseStrictDouble(t1S, t1) ||
            !parseStrictDouble(zS, z) ||
            !parseStrictDouble(erS, er) ||
            !parseStrictDouble(muS, mu) ||
            !parseStrictDouble(sdS, sd) ||
            !parseStrictInt(winsS, wins)) {
            throw std::runtime_error("PersistenceManager: malformed STATISTICAL_ANOMALY record.");
        }
        return new StatisticalAnomalyEvent(ch, unit, i0, i1, t0, t1, sev, z, er, mu, sd, wins);
    }

    throw std::runtime_error("PersistenceManager: unknown event type \"" + type + "\".");
}

ChannelSignal PersistenceManager::loadSingleColumnTxt(const std::string& filepath,
                                                      const std::string& channelCode,
                                                      const std::string& unit,
                                                      double samplingRate,
                                                      int maxSamples,
                                                      IngestionStats* outStats) {
    if (maxSamples < 0) {
        throw std::invalid_argument("PersistenceManager::loadSingleColumnTxt: maxSamples cannot be negative.");
    }

    std::ifstream in(filepath);
    if (!in.is_open()) {
        throw std::runtime_error("PersistenceManager::loadSingleColumnTxt: cannot open \"" + filepath + "\".");
    }

    IngestionStats stats;
    int capacity = kInitialSampleCapacity;
    int count = 0;
    double* buffer = new double[capacity];

    try {
        std::string line;
        while (std::getline(in, line)) {
            ++stats.totalLinesRead;
            double sample = 0.0;
            if (!parseStrictDouble(line, sample)) {
                ++stats.skippedLines;
                continue;
            }

            if (count == capacity) {
                capacity *= 2;
                buffer = growDoubleBuffer(buffer, count, capacity);
            }
            *(buffer + count) = sample;
            ++count;

            if (maxSamples > 0 && count >= maxSamples) {
                break;
            }
        }
    } catch (...) {
        delete[] buffer;
        throw;
    }

    stats.loadedSamples = count;
    if (outStats != nullptr) {
        *outStats = stats;
    }

    if (count < 2) {
        delete[] buffer;
        throw std::runtime_error("PersistenceManager::loadSingleColumnTxt: fewer than 2 valid samples in \"" +
                                 filepath + "\".");
    }

    ChannelSignal signal(channelCode, unit, samplingRate, buffer, count);
    delete[] buffer;
    return signal;
}

ChannelSignal PersistenceManager::loadCsvColumn(const std::string& filepath,
                                                int columnIndex,
                                                bool hasHeader,
                                                const std::string& channelCode,
                                                const std::string& unit,
                                                double samplingRate,
                                                char delimiter,
                                                int maxSamples,
                                                IngestionStats* outStats) {
    if (columnIndex < 0 || maxSamples < 0) {
        throw std::invalid_argument("PersistenceManager::loadCsvColumn: invalid columnIndex or maxSamples.");
    }

    std::ifstream in(filepath);
    if (!in.is_open()) {
        throw std::runtime_error("PersistenceManager::loadCsvColumn: cannot open \"" + filepath + "\".");
    }

    IngestionStats stats;
    bool headerPending = hasHeader;
    int capacity = kInitialSampleCapacity;
    int count = 0;
    double* buffer = new double[capacity];

    try {
        std::string line;
        while (std::getline(in, line)) {
            ++stats.totalLinesRead;
            const std::string cleaned = trim(line);
            if (cleaned.empty()) {
                ++stats.skippedLines;
                continue;
            }
            if (headerPending) {
                headerPending = false;
                continue;
            }

            std::string field;
            double sample = 0.0;
            if (!extractDelimitedField(cleaned, columnIndex, delimiter, field) ||
                !parseStrictDouble(field, sample)) {
                ++stats.skippedLines;
                continue;
            }

            if (count == capacity) {
                capacity *= 2;
                buffer = growDoubleBuffer(buffer, count, capacity);
            }
            *(buffer + count) = sample;
            ++count;

            if (maxSamples > 0 && count >= maxSamples) {
                break;
            }
        }
    } catch (...) {
        delete[] buffer;
        throw;
    }

    stats.loadedSamples = count;
    if (outStats != nullptr) {
        *outStats = stats;
    }

    if (count < 2) {
        delete[] buffer;
        throw std::runtime_error("PersistenceManager::loadCsvColumn: fewer than 2 valid samples in \"" +
                                 filepath + "\".");
    }

    ChannelSignal signal(channelCode, unit, samplingRate, buffer, count);
    delete[] buffer;
    return signal;
}

int PersistenceManager::loadMultiChannelCsv(const std::string& filepath,
                                            StudySession& session,
                                            const std::string* channelCodes,
                                            const int* columnIndices,
                                            int channelCount,
                                            const std::string& unit,
                                            double samplingRate,
                                            bool hasHeader,
                                            char delimiter,
                                            int maxSamples,
                                            IngestionStats* outStats) {
    if (channelCount <= 0 || channelCodes == nullptr || columnIndices == nullptr || maxSamples < 0) {
        throw std::invalid_argument("PersistenceManager::loadMultiChannelCsv: invalid arguments.");
    }
    for (int c = 0; c < channelCount; ++c) {
        if (*(columnIndices + c) < 0) {
            throw std::invalid_argument("PersistenceManager::loadMultiChannelCsv: negative column index.");
        }
    }

    std::ifstream in(filepath);
    if (!in.is_open()) {
        throw std::runtime_error("PersistenceManager::loadMultiChannelCsv: cannot open \"" + filepath + "\".");
    }

    IngestionStats stats;
    bool headerPending = hasHeader;
    int capacity = kInitialSampleCapacity;
    int count = 0;

    double** buffers = new double*[channelCount]();
    double* rowValues = new double[channelCount]();

    try {
        for (int c = 0; c < channelCount; ++c) {
            *(buffers + c) = new double[capacity];
        }

        std::string line;
        while (std::getline(in, line)) {
            ++stats.totalLinesRead;
            const std::string cleaned = trim(line);
            if (cleaned.empty()) {
                ++stats.skippedLines;
                continue;
            }
            if (headerPending) {
                headerPending = false;
                continue;
            }

            bool rowValid = true;
            for (int c = 0; c < channelCount; ++c) {
                std::string field;
                if (!extractDelimitedField(cleaned, *(columnIndices + c), delimiter, field) ||
                    !parseStrictDouble(field, *(rowValues + c))) {
                    rowValid = false;
                    break;
                }
            }

            if (!rowValid) {
                ++stats.skippedLines;
                continue;
            }

            if (count == capacity) {
                const int newCap = capacity * 2;
                for (int c = 0; c < channelCount; ++c) {
                    *(buffers + c) = growDoubleBuffer(*(buffers + c), count, newCap);
                }
                capacity = newCap;
            }

            for (int c = 0; c < channelCount; ++c) {
                *(*(buffers + c) + count) = *(rowValues + c);
            }
            ++count;

            if (maxSamples > 0 && count >= maxSamples) {
                break;
            }
        }

        stats.loadedSamples = count;
        if (outStats != nullptr) {
            *outStats = stats;
        }

        if (count < 2) {
            throw std::runtime_error("PersistenceManager::loadMultiChannelCsv: fewer than 2 valid rows in \"" +
                                     filepath + "\".");
        }

        for (int c = 0; c < channelCount; ++c) {
            ChannelSignal sig(*(channelCodes + c), unit, samplingRate, *(buffers + c), count);
            session.addChannel(sig);
        }
    } catch (...) {
        delete[] rowValues;
        for (int c = 0; c < channelCount; ++c) {
            delete[] *(buffers + c);
        }
        delete[] buffers;
        throw;
    }

    delete[] rowValues;
    for (int c = 0; c < channelCount; ++c) {
        delete[] *(buffers + c);
    }
    delete[] buffers;

    return channelCount;
}

bool PersistenceManager::loadMitBihBeatAnnotations(const std::string& filepath,
                                                   int*& outIndices,
                                                   int& outCount,
                                                   bool hasHeader) {
    outIndices = nullptr;
    outCount = 0;

    std::ifstream in(filepath);
    if (!in.is_open()) {
        return false;
    }

    int capacity = 32;
    int count = 0;
    int* buffer = new int[capacity];
    bool headerPending = hasHeader;

    try {
        std::string line;
        while (std::getline(in, line)) {
            const std::string cleaned = trim(line);
            if (cleaned.empty()) {
                continue;
            }
            if (headerPending) {
                headerPending = false;
                continue;
            }

            std::string idxStr;
            std::string symStr;
            int idx = 0;
            if (!extractDelimitedField(cleaned, 0, ',', idxStr) ||
                !extractDelimitedField(cleaned, 1, ',', symStr) ||
                !parseStrictInt(idxStr, idx) ||
                idx < 0) {
                continue;
            }

            const std::string sym = trim(symStr);
            if (sym.empty() || sym == "+" || sym == "~" || sym == "|" || sym == "x") {
                continue;
            }

            if (count == capacity) {
                const int newCap = capacity * 2;
                int* grown = new int[newCap];
                for (int i = 0; i < count; ++i) {
                    *(grown + i) = *(buffer + i);
                }
                delete[] buffer;
                buffer = grown;
                capacity = newCap;
            }

            *(buffer + count) = idx;
            ++count;
        }
    } catch (...) {
        delete[] buffer;
        throw;
    }

    if (count == 0) {
        delete[] buffer;
        outIndices = nullptr;
        outCount = 0;
        return true;
    }

    outIndices = buffer;
    outCount = count;
    return true;
}

void PersistenceManager::saveSession(const StudySession& session, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) {
        throw std::runtime_error("PersistenceManager::saveSession: cannot open \"" + filepath + "\" for writing.");
    }

    out << std::setprecision(10);
    out << "[STUDY_SESSION_V1]\n";
    out << "NAME;" << session.getSessionName() << "\n";

    const ProcessingPipeline& pipe = session.getPipeline();
    out << "[PIPELINE];" << pipe.getCount() << "\n";
    for (int i = 0; i < pipe.getCount(); ++i) {
        out << pipe[i].serialize() << "\n";
    }

    const DetectionEngine& eng = session.getEngine();
    out << "[DETECTORS];" << eng.getDetectorCount() << "\n";
    for (int i = 0; i < eng.getDetectorCount(); ++i) {
        out << serializeDetector(eng.getDetector(i)) << "\n";
    }

    out << "[CHANNELS];" << session.getChannelCount() << "\n";
    for (int c = 0; c < session.getChannelCount(); ++c) {
        const ChannelSignal& ch = session.getChannel(c);
        const MetadataSensor& meta = ch.getMetadata();
        const int n = ch.getNumberOfSamples();

        out << "CHANNEL;code=" << meta.getCodeOfChannel()
            << ";unit=" << meta.getUnitOfMeasure()
            << ";fs=" << meta.getSamplingRate()
            << ";n=" << n << "\n";

        out << "RAW;";
        for (int i = 0; i < n; ++i) {
            if (i > 0) {
                out << ",";
            }
            out << ch.getRawElement(i);
        }
        out << "\n";

        out << "PROCESSED;";
        for (int i = 0; i < n; ++i) {
            if (i > 0) {
                out << ",";
            }
            out << ch.getProcessedElement(i);
        }
        out << "\n";
    }

    const EventRegistry& reg = session.getRegistry();
    out << "[EVENTS];" << reg.getCount() << "\n";
    for (int i = 0; i < reg.getCount(); ++i) {
        out << serializeEvent(reg[i]) << "\n";
    }

    out << "[END_SESSION]\n";
    out.flush();
    if (!out.good()) {
        throw std::runtime_error("PersistenceManager::saveSession: stream write error on \"" + filepath + "\".");
    }
}

void PersistenceManager::loadSession(const std::string& filepath, StudySession& outSession) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        throw std::runtime_error("PersistenceManager::loadSession: cannot open \"" + filepath + "\".");
    }

    std::string line;
    if (!readDataLine(in, line) || line != "[STUDY_SESSION_V1]") {
        throw std::runtime_error("PersistenceManager::loadSession: invalid or missing session header.");
    }

    if (!readDataLine(in, line) || line.compare(0, 5, "NAME;") != 0) {
        throw std::runtime_error("PersistenceManager::loadSession: missing session NAME field.");
    }
    StudySession loaded(line.substr(5), 4);

    if (!readDataLine(in, line) || line.compare(0, 11, "[PIPELINE];") != 0) {
        throw std::runtime_error("PersistenceManager::loadSession: missing [PIPELINE] header.");
    }
    int stageCount = 0;
    if (!parseStrictInt(line.substr(11), stageCount) || stageCount < 0) {
        throw std::runtime_error("PersistenceManager::loadSession: invalid pipeline stage count.");
    }
    for (int i = 0; i < stageCount; ++i) {
        if (!readDataLine(in, line)) {
            throw std::runtime_error("PersistenceManager::loadSession: truncated pipeline stages.");
        }
        ProcessingModule* mod = deserializeModule(line);
        loaded.getPipeline().adoptStage(mod);
    }

    if (!readDataLine(in, line) || line.compare(0, 12, "[DETECTORS];") != 0) {
        throw std::runtime_error("PersistenceManager::loadSession: missing [DETECTORS] header.");
    }
    int detectorCount = 0;
    if (!parseStrictInt(line.substr(12), detectorCount) || detectorCount < 0) {
        throw std::runtime_error("PersistenceManager::loadSession: invalid detector count.");
    }
    for (int i = 0; i < detectorCount; ++i) {
        if (!readDataLine(in, line)) {
            throw std::runtime_error("PersistenceManager::loadSession: truncated detector entries.");
        }
        EventDetector* det = deserializeDetector(line);
        loaded.getEngine().adoptDetector(det);
    }

    if (!readDataLine(in, line) || line.compare(0, 11, "[CHANNELS];") != 0) {
        throw std::runtime_error("PersistenceManager::loadSession: missing [CHANNELS] header.");
    }
    int channelCount = 0;
    if (!parseStrictInt(line.substr(11), channelCount) || channelCount < 0) {
        throw std::runtime_error("PersistenceManager::loadSession: invalid channel count.");
    }

    for (int c = 0; c < channelCount; ++c) {
        if (!readDataLine(in, line) || line.compare(0, 8, "CHANNEL;") != 0) {
            throw std::runtime_error("PersistenceManager::loadSession: missing CHANNEL descriptor.");
        }

        std::string code, unit, fsS, nS;
        double fs = 1.0;
        int n = 0;
        if (!extractKeyValue(line, "code", code) ||
            !extractKeyValue(line, "unit", unit) ||
            !extractKeyValue(line, "fs", fsS) ||
            !extractKeyValue(line, "n", nS) ||
            !parseStrictDouble(fsS, fs) ||
            !parseStrictInt(nS, n) ||
            n < 2) {
            throw std::runtime_error("PersistenceManager::loadSession: malformed CHANNEL metadata.");
        }

        std::string rawLine;
        std::string procLine;
        if (!readDataLine(in, rawLine) || rawLine.compare(0, 4, "RAW;") != 0 ||
            !readDataLine(in, procLine) || procLine.compare(0, 10, "PROCESSED;") != 0) {
            throw std::runtime_error("PersistenceManager::loadSession: missing RAW/PROCESSED buffers.");
        }

        double* rawBuf = new double[n];
        double* procBuf = new double[n];
        try {
            const std::string rawPayload = rawLine.substr(4);
            const std::string procPayload = procLine.substr(10);

            std::size_t rawPos = 0;
            std::size_t procPos = 0;
            for (int i = 0; i < n; ++i) {
                std::size_t rawSep = rawPayload.find(',', rawPos);
                if (rawSep == std::string::npos) {
                    rawSep = rawPayload.size();
                }
                if (rawPos >= rawPayload.size() ||
                    !parseStrictDouble(rawPayload.substr(rawPos, rawSep - rawPos), *(rawBuf + i))) {
                    throw std::runtime_error("PersistenceManager::loadSession: corrupt RAW sample.");
                }
                rawPos = (rawSep < rawPayload.size()) ? (rawSep + 1) : rawPayload.size();

                std::size_t procSep = procPayload.find(',', procPos);
                if (procSep == std::string::npos) {
                    procSep = procPayload.size();
                }
                if (procPos >= procPayload.size() ||
                    !parseStrictDouble(procPayload.substr(procPos, procSep - procPos), *(procBuf + i))) {
                    throw std::runtime_error("PersistenceManager::loadSession: corrupt PROCESSED sample.");
                }
                procPos = (procSep < procPayload.size()) ? (procSep + 1) : procPayload.size();
            }

            ChannelSignal ch(code, unit, fs, rawBuf, n);
            for (int i = 0; i < n; ++i) {
                ch.setProcessedElement(*(procBuf + i), i);
            }
            loaded.addChannel(ch);
        } catch (...) {
            delete[] rawBuf;
            delete[] procBuf;
            throw;
        }

        delete[] rawBuf;
        delete[] procBuf;
    }

    if (!readDataLine(in, line) || line.compare(0, 9, "[EVENTS];") != 0) {
        throw std::runtime_error("PersistenceManager::loadSession: missing [EVENTS] header.");
    }
    int eventCount = 0;
    if (!parseStrictInt(line.substr(9), eventCount) || eventCount < 0) {
        throw std::runtime_error("PersistenceManager::loadSession: invalid event count.");
    }
    for (int i = 0; i < eventCount; ++i) {
        if (!readDataLine(in, line)) {
            throw std::runtime_error("PersistenceManager::loadSession: truncated event entries.");
        }
        DetectedEvent* ev = deserializeEvent(line);
        loaded.getRegistry().adopt(ev);
    }

    if (!readDataLine(in, line) || line != "[END_SESSION]") {
        throw std::runtime_error("PersistenceManager::loadSession: missing [END_SESSION] sentinel.");
    }

    outSession = loaded;
}

void PersistenceManager::writeReport(const StudySession& session,
                                     std::ostream& os,
                                     int maxEventsToDisplay) {
    const std::ios::fmtflags savedFlags = os.flags();
    const std::streamsize savedPrec = os.precision();
    const char savedFill = os.fill();

    const std::string heavyRule(static_cast<std::size_t>(kReportWidth), '=');
    const std::string lightRule(static_cast<std::size_t>(kReportWidth), '-');

    os << heavyRule << "\n";
    os << std::left << std::setw(kReportWidth)
       << truncateField("  TIME-SERIES STUDY REPORT :: " + session.getSessionName(), kReportWidth)
       << "\n";
    os << heavyRule << "\n";

    os << "Channels: " << std::setw(4) << std::left << session.getChannelCount()
       << " | Pipeline Stages: " << std::setw(3) << session.getPipeline().getCount()
       << " | Detectors: " << std::setw(3) << session.getEngine().getDetectorCount()
       << " | Events: " << session.getRegistry().getCount() << "\n";

    os << lightRule << "\n";
    os << "1. CHANNELS BASELINE (PROCESSED BUFFER)\n";
    os << lightRule << "\n";
    os << std::left << std::setw(12) << "Channel"
       << std::left << std::setw(6) << "Unit"
       << std::right << std::setw(8) << "Fs(Hz)"
       << std::right << std::setw(7) << "N"
       << std::right << std::setw(12) << "Mean"
       << std::right << std::setw(11) << "StdDev"
       << std::right << std::setw(12) << "Min"
       << std::right << std::setw(12) << "Max" << "\n";
    os << lightRule << "\n";

    os << std::fixed;
    if (session.getChannelCount() == 0) {
        os << "  (No channels loaded)\n";
    } else {
        for (int i = 0; i < session.getChannelCount(); ++i) {
            const ChannelSignal& ch = session.getChannel(i);
            const MetadataSensor& meta = ch.getMetadata();
            const ChannelStadistics& st = ch.getProcessedStatistics();
            os << std::left << std::setw(12) << truncateField(meta.getCodeOfChannel(), 11)
               << std::left << std::setw(6) << truncateField(meta.getUnitOfMeasure(), 5)
               << std::right << std::setw(8) << std::setprecision(2) << meta.getSamplingRate()
               << std::right << std::setw(7) << ch.getNumberOfSamples()
               << std::right << std::setw(12) << std::setprecision(2) << st.getAverage()
               << std::right << std::setw(11) << std::setprecision(2) << st.getStandardDesviation()
               << std::right << std::setw(12) << std::setprecision(2) << st.getMinimum()
               << std::right << std::setw(12) << std::setprecision(2) << st.getMaximum()
               << "\n";
        }
    }

    os << lightRule << "\n";
    os << "2. PROCESSING PIPELINE\n";
    os << lightRule << "\n";
    const ProcessingPipeline& pipe = session.getPipeline();
    if (pipe.isEmpty()) {
        os << "  (Empty pipeline - raw signal passthrough)\n";
    } else {
        for (int i = 0; i < pipe.getCount(); ++i) {
            const std::string line = "  [" + std::to_string(i) + "] " + pipe[i].serialize();
            os << truncateField(line, kReportWidth) << "\n";
        }
    }

    os << lightRule << "\n";
    os << "3. DETECTED EVENTS TRIAGE TABLE\n";
    os << lightRule << "\n";
    os << std::left << std::setw(5) << "#"
       << std::left << std::setw(14) << "Type"
       << std::left << std::setw(11) << "Channel"
       << std::left << std::setw(14) << "Idx[i0..i1]"
       << std::right << std::setw(10) << "Time(s)"
       << std::right << std::setw(13) << "Metric"
       << std::right << std::setw(13) << "Severity" << "\n";
    os << lightRule << "\n";

    const EventRegistry& reg = session.getRegistry();
    const int totalEvents = reg.getCount();
    const int limit = (maxEventsToDisplay > 0 && maxEventsToDisplay < totalEvents)
                          ? maxEventsToDisplay
                          : totalEvents;

    if (totalEvents == 0) {
        os << "  (No events detected)\n";
    } else {
        for (int i = 0; i < limit; ++i) {
            const DetectedEvent& ev = reg[i];
            const std::string idxRange = "[" + std::to_string(ev.getInitIndex()) + ".." +
                                         std::to_string(ev.getEndIndex()) + "]";
            std::string shortType = ev.getType();
            if (shortType == "STATISTICAL_ANOMALY") {
                shortType = "STAT_ANOMALY";
            }

            os << std::left << std::setw(5) << (i + 1)
               << std::left << std::setw(14) << truncateField(shortType, 13)
               << std::left << std::setw(11) << truncateField(ev.getCodeOfChannel(), 10)
               << std::left << std::setw(14) << truncateField(idxRange, 13)
               << std::right << std::setw(10) << std::setprecision(3) << ev.getStartTimeInSecs()
               << std::right << std::setw(13) << std::setprecision(3) << ev.getCharacteristicValue()
               << std::right << std::setw(13) << std::setprecision(3) << ev.getSeverity()
               << "\n";
        }
        if (limit < totalEvents) {
            os << "  ... (" << (totalEvents - limit) << " additional events omitted)\n";
        }
    }

    os << heavyRule << "\n";

    os.flags(savedFlags);
    os.precision(savedPrec);
    os.fill(savedFill);
}

void PersistenceManager::exportReport(const StudySession& session,
                                      const std::string& filepath,
                                      int maxEventsToDisplay) {
    std::ofstream out(filepath);
    if (!out.is_open()) {
        throw std::runtime_error("PersistenceManager::exportReport: cannot open \"" + filepath + "\" for writing.");
    }
    writeReport(session, out, maxEventsToDisplay);
    out.flush();
    if (!out.good()) {
        throw std::runtime_error("PersistenceManager::exportReport: stream write error on \"" + filepath + "\".");
    }
}
