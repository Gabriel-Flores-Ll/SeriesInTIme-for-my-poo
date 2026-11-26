#include "PeakDetector.h"
#include "PeakEvent.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

PeakDetector::PeakDetector(const std::string& givenName,
                           double givenMinAmplitude,
                           double givenMinProminence,
                           int givenMinDistanceSamples,
                           bool givenDetectValleys,
                           const std::string& givenBufferSelector)
    : EventDetector(givenName, givenBufferSelector),
      minAmplitude(givenMinAmplitude),
      minProminence(givenMinProminence),
      minDistanceSamples(givenMinDistanceSamples),
      detectValleys(givenDetectValleys) {
    if (!std::isfinite(minAmplitude) || minAmplitude < 0.0) {
        throw std::invalid_argument("PeakDetector: minAmplitude must be finite and non-negative.");
    }
    if (!std::isfinite(minProminence) || minProminence < 0.0) {
        throw std::invalid_argument("PeakDetector: minProminence must be finite and non-negative.");
    }
    if (minDistanceSamples < 1) {
        throw std::invalid_argument("PeakDetector: minDistanceSamples must be >= 1.");
    }
}

EventRegistry PeakDetector::detect(const ChannelSignal& signal) const {
    EventRegistry registry(0);
    const int n = signal.getNumberOfSamples();
    if (n < 3) {
        return registry;
    }

    double mean = 0.0;
    double stdDev = 0.0;
    double totalEnergy = 0.0;
    computeBaseline(signal, mean, stdDev, totalEnergy);

    const double severityScale = (stdDev >= kBaselineEpsilon) ? stdDev : 1.0;
    const MetadataSensor& meta = signal.getMetadata();

    bool hasPending = false;
    int pendingIndex = 0;
    double pendingAmplitude = 0.0;
    double pendingProminence = 0.0;
    bool pendingPositive = true;

    int k = 1;
    while (k < n - 1) {
        const double prev = sampleAt(signal, k - 1);
        const double curr = sampleAt(signal, k);

        bool isCandidate = false;
        bool isPositive = true;
        int nextDiff = k + 1;

        if (curr > prev) {
            while (nextDiff < n && sampleAt(signal, nextDiff) == curr) {
                ++nextDiff;
            }
            if (nextDiff < n && curr > sampleAt(signal, nextDiff)) {
                isCandidate = true;
                isPositive = true;
            }
        } else if (detectValleys && curr < prev) {
            while (nextDiff < n && sampleAt(signal, nextDiff) == curr) {
                ++nextDiff;
            }
            if (nextDiff < n && curr < sampleAt(signal, nextDiff)) {
                isCandidate = true;
                isPositive = false;
            }
        }

        if (isCandidate) {
            const double prominence = std::fabs(curr - mean);
            if (std::fabs(curr) >= minAmplitude && prominence >= minProminence) {
                if (!hasPending) {
                    hasPending = true;
                    pendingIndex = k;
                    pendingAmplitude = curr;
                    pendingProminence = prominence;
                    pendingPositive = isPositive;
                } else if (k - pendingIndex < minDistanceSamples) {
                    if (prominence > pendingProminence) {
                        pendingIndex = k;
                        pendingAmplitude = curr;
                        pendingProminence = prominence;
                        pendingPositive = isPositive;
                    }
                } else {
                    const double severity = pendingProminence / severityScale;
                    registry.adopt(new PeakEvent(meta.getCodeOfChannel(),
                                                 meta.getUnitOfMeasure(),
                                                 pendingIndex,
                                                 meta.ConvertIndexSecs(pendingIndex),
                                                 severity,
                                                 pendingAmplitude,
                                                 pendingProminence,
                                                 pendingPositive));
                    pendingIndex = k;
                    pendingAmplitude = curr;
                    pendingProminence = prominence;
                    pendingPositive = isPositive;
                }
            }
            k = nextDiff;
        } else {
            ++k;
        }
    }

    if (hasPending) {
        const double severity = pendingProminence / severityScale;
        registry.adopt(new PeakEvent(meta.getCodeOfChannel(),
                                     meta.getUnitOfMeasure(),
                                     pendingIndex,
                                     meta.ConvertIndexSecs(pendingIndex),
                                     severity,
                                     pendingAmplitude,
                                     pendingProminence,
                                     pendingPositive));
    }

    return registry;
}

EventDetector* PeakDetector::clone() const {
    return new PeakDetector(*this);
}

std::string PeakDetector::getDescription() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << "PeakDetector(name=" << getName()
        << ", buffer=" << getBufferSelector()
        << ", minAmp=" << minAmplitude
        << ", minProm=" << minProminence
        << ", minDist=" << minDistanceSamples
        << ", valleys=" << (detectValleys ? "true" : "false") << ")";
    return oss.str();
}

double PeakDetector::getMinAmplitude() const {
    return minAmplitude;
}

double PeakDetector::getMinProminence() const {
    return minProminence;
}

int PeakDetector::getMinDistanceSamples() const {
    return minDistanceSamples;
}

bool PeakDetector::isDetectingValleys() const {
    return detectValleys;
}