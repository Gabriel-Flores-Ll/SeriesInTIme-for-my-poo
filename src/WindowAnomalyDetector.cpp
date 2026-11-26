#include "WindowAnomalyDetector.h"
#include "SlicingWindow.h"
#include "StatisticalAnomalyEvent.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

WindowAnomalyDetector::WindowAnomalyDetector(const std::string& givenName,
                                             int givenWindowSize,
                                             int givenStride,
                                             double givenZScoreThreshold,
                                             double givenEnergyRatioThreshold,
                                             const std::string& givenBufferSelector)
    : EventDetector(givenName, givenBufferSelector),
      windowSize(givenWindowSize),
      stride(givenStride),
      zScoreThreshold(givenZScoreThreshold),
      energyRatioThreshold(givenEnergyRatioThreshold) {
    if (windowSize < 1) {
        throw std::invalid_argument("WindowAnomalyDetector: windowSize must be >= 1.");
    }
    if (stride < 1) {
        throw std::invalid_argument("WindowAnomalyDetector: stride must be >= 1.");
    }
    if (!std::isfinite(zScoreThreshold) || zScoreThreshold <= 0.0) {
        throw std::invalid_argument("WindowAnomalyDetector: zScoreThreshold must be finite and > 0.0.");
    }
    if (!std::isfinite(energyRatioThreshold) || energyRatioThreshold <= 0.0) {
        throw std::invalid_argument("WindowAnomalyDetector: energyRatioThreshold must be finite and > 0.0.");
    }
}

EventRegistry WindowAnomalyDetector::detect(const ChannelSignal& signal) const {
    EventRegistry registry(0);
    const int n = signal.getNumberOfSamples();
    if (n < windowSize) {
        return registry;
    }

    double mean = 0.0;
    double stdDev = 0.0;
    double totalEnergy = 0.0;
    computeBaseline(signal, mean, stdDev, totalEnergy);

    const double expectedWindowEnergy =
        totalEnergy * (static_cast<double>(windowSize) / static_cast<double>(n));
    const MetadataSensor& meta = signal.getMetadata();

    SlicingWindow window(&signal, windowSize, stride);

    bool inEpisode = false;
    int epStart = 0;
    int epEnd = 0;
    int epWindows = 0;
    double epMaxZ = 0.0;
    double epMaxEnergyRatio = 0.0;

    auto flushEpisode = [&]() {
        const double absZ = std::fabs(epMaxZ);
        const double rmsRatio = std::sqrt(epMaxEnergyRatio);
        const double severity = (absZ > rmsRatio) ? absZ : rmsRatio;

        registry.adopt(new StatisticalAnomalyEvent(meta.getCodeOfChannel(),
                                                   meta.getUnitOfMeasure(),
                                                   epStart,
                                                   epEnd,
                                                   meta.ConvertIndexSecs(epStart),
                                                   meta.ConvertIndexSecs(epEnd),
                                                   severity,
                                                   epMaxZ,
                                                   epMaxEnergyRatio,
                                                   mean,
                                                   stdDev,
                                                   epWindows));
        inEpisode = false;
    };

    do {
        const int wStart = window.getInitIndex();
        const int wEnd = wStart + window.getWindowLenght() - 1;

        const double wMean = window.averageWindow(getBufferSelector());
        const double wEnergy = window.energyWindow(getBufferSelector());

        const double zScore = (stdDev >= kBaselineEpsilon) ? ((wMean - mean) / stdDev) : 0.0;
        const double energyRatio =
            (expectedWindowEnergy >= kBaselineEpsilon) ? (wEnergy / expectedWindowEnergy) : 0.0;

        const bool isAnomalous =
            (std::fabs(zScore) >= zScoreThreshold) || (energyRatio >= energyRatioThreshold);

        if (isAnomalous) {
            if (!inEpisode) {
                inEpisode = true;
                epStart = wStart;
                epEnd = wEnd;
                epWindows = 1;
                epMaxZ = zScore;
                epMaxEnergyRatio = energyRatio;
            } else if (wStart <= epEnd + 1) {
                epEnd = wEnd;
                ++epWindows;
                if (std::fabs(zScore) > std::fabs(epMaxZ)) {
                    epMaxZ = zScore;
                }
                if (energyRatio > epMaxEnergyRatio) {
                    epMaxEnergyRatio = energyRatio;
                }
            } else {
                flushEpisode();
                inEpisode = true;
                epStart = wStart;
                epEnd = wEnd;
                epWindows = 1;
                epMaxZ = zScore;
                epMaxEnergyRatio = energyRatio;
            }
        } else if (inEpisode) {
            flushEpisode();
        }
    } while (window.strideForward());

    if (inEpisode) {
        flushEpisode();
    }

    return registry;
}

EventDetector* WindowAnomalyDetector::clone() const {
    return new WindowAnomalyDetector(*this);
}

std::string WindowAnomalyDetector::getDescription() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << "WindowAnomalyDetector(name=" << getName()
        << ", buffer=" << getBufferSelector()
        << ", W=" << windowSize
        << ", S=" << stride
        << ", zThr=" << zScoreThreshold
        << ", eRatioThr=" << energyRatioThreshold << ")";
    return oss.str();
}

int WindowAnomalyDetector::getWindowSize() const {
    return windowSize;
}

int WindowAnomalyDetector::getStride() const {
    return stride;
}

double WindowAnomalyDetector::getZScoreThreshold() const {
    return zScoreThreshold;
}

double WindowAnomalyDetector::getEnergyRatioThreshold() const {
    return energyRatioThreshold;
}