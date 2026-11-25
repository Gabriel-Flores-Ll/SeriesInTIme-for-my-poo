#include "ThresholdDetector.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

ThresholdDetector::ThresholdDetector(const std::string& givenName,
                                     double givenActivationThreshold,
                                     double givenReleaseThreshold,
                                     int givenMinDurationSamples,
                                     ThresholdEvent::CrossingType givenCrossingType,
                                     const std::string& givenBufferSelector)
    : EventDetector(givenName, givenBufferSelector),
      activationThreshold(givenActivationThreshold),
      releaseThreshold(givenReleaseThreshold),
      minDurationSamples(givenMinDurationSamples),
      crossingType(givenCrossingType) {
    if (!std::isfinite(activationThreshold) || !std::isfinite(releaseThreshold)) {
        throw std::invalid_argument("ThresholdDetector: thresholds must be finite.");
    }
    if (crossingType == ThresholdEvent::CrossingType::UPPER) {
        if (releaseThreshold > activationThreshold) {
            throw std::invalid_argument("ThresholdDetector: UPPER crossing requires releaseThreshold <= activationThreshold.");
        }
    } else if (crossingType == ThresholdEvent::CrossingType::LOWER) {
        if (releaseThreshold < activationThreshold) {
            throw std::invalid_argument("ThresholdDetector: LOWER crossing requires releaseThreshold >= activationThreshold.");
        }
    } else {
        throw std::invalid_argument("ThresholdDetector: invalid CrossingType.");
    }
    if (minDurationSamples < 1) {
        throw std::invalid_argument("ThresholdDetector: minDurationSamples must be >= 1.");
    }
}

EventRegistry ThresholdDetector::detect(const ChannelSignal& signal) const {
    EventRegistry registry(0);
    const int n = signal.getNumberOfSamples();
    if (n <= 0) {
        return registry;
    }

    double mean = 0.0;
    double stdDev = 0.0;
    double totalEnergy = 0.0;
    computeBaseline(signal, mean, stdDev, totalEnergy);

    const double severityScale = (stdDev >= kBaselineEpsilon) ? stdDev : 1.0;
    const MetadataSensor& meta = signal.getMetadata();

    bool inExcursion = false;
    int startIdx = 0;
    double extremeVal = 0.0;

    for (int k = 0; k < n; ++k) {
        const double val = sampleAt(signal, k);

        if (!inExcursion) {
            const bool activated =
                (crossingType == ThresholdEvent::CrossingType::UPPER)
                    ? (val >= activationThreshold)
                    : (val <= activationThreshold);
            if (activated) {
                inExcursion = true;
                startIdx = k;
                extremeVal = val;
            }
        } else {
            const bool released =
                (crossingType == ThresholdEvent::CrossingType::UPPER)
                    ? (val < releaseThreshold)
                    : (val > releaseThreshold);
            if (released) {
                const int endIdx = k - 1;
                if (endIdx - startIdx + 1 >= minDurationSamples) {
                    const double excess = std::fabs(extremeVal - activationThreshold);
                    const double severity = excess / severityScale;
                    registry.adopt(new ThresholdEvent(meta.getCodeOfChannel(),
                                                      meta.getUnitOfMeasure(),
                                                      startIdx,
                                                      endIdx,
                                                      meta.ConvertIndexSecs(startIdx),
                                                      meta.ConvertIndexSecs(endIdx),
                                                      severity,
                                                      activationThreshold,
                                                      extremeVal,
                                                      crossingType));
                }
                inExcursion = false;
            } else {
                if (crossingType == ThresholdEvent::CrossingType::UPPER) {
                    if (val > extremeVal) {
                        extremeVal = val;
                    }
                } else {
                    if (val < extremeVal) {
                        extremeVal = val;
                    }
                }
            }
        }
    }

    if (inExcursion) {
        const int endIdx = n - 1;
        if (endIdx - startIdx + 1 >= minDurationSamples) {
            const double excess = std::fabs(extremeVal - activationThreshold);
            const double severity = excess / severityScale;
            registry.adopt(new ThresholdEvent(meta.getCodeOfChannel(),
                                              meta.getUnitOfMeasure(),
                                              startIdx,
                                              endIdx,
                                              meta.ConvertIndexSecs(startIdx),
                                              meta.ConvertIndexSecs(endIdx),
                                              severity,
                                              activationThreshold,
                                              extremeVal,
                                              crossingType));
        }
    }

    return registry;
}

EventDetector* ThresholdDetector::clone() const {
    return new ThresholdDetector(*this);
}

std::string ThresholdDetector::getDescription() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << "ThresholdDetector(name=" << getName()
        << ", buffer=" << getBufferSelector()
        << ", crossing=" << (crossingType == ThresholdEvent::CrossingType::UPPER ? "UPPER" : "LOWER")
        << ", act=" << activationThreshold
        << ", rel=" << releaseThreshold
        << ", minDur=" << minDurationSamples << ")";
    return oss.str();
}

double ThresholdDetector::getActivationThreshold() const {
    return activationThreshold;
}

double ThresholdDetector::getReleaseThreshold() const {
    return releaseThreshold;
}

int ThresholdDetector::getMinDurationSamples() const {
    return minDurationSamples;
}

ThresholdEvent::CrossingType ThresholdDetector::getCrossingType() const {
    return crossingType;
}