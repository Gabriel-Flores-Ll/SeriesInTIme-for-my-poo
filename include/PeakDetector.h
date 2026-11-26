#ifndef PEAK_DETECTOR_H
#define PEAK_DETECTOR_H

#include "EventDetector.h"

#include <string>

/**
 * @class PeakDetector
 * @brief Concrete detector identifying local extrema (positive peaks and optionally negative valleys)
 *        enforcing amplitude, baseline prominence, and refractory distance constraints.
 *        Follows the Rule of Zero.
 * @invariant minAmplitude >= 0.0 and std::isfinite(minAmplitude).
 * @invariant minProminence >= 0.0 and std::isfinite(minProminence).
 * @invariant minDistanceSamples >= 1.
 */
class PeakDetector : public EventDetector {
private:
    double minAmplitude;
    double minProminence;
    int minDistanceSamples;
    bool detectValleys;

public:
    /**
     * @brief Constructs a validated PeakDetector.
     * @param givenName Non-empty detector identifier.
     * @param givenMinAmplitude Minimum required absolute amplitude |x[k]| (>= 0.0, finite).
     * @param givenMinProminence Minimum required baseline prominence |x[k] - mu| (>= 0.0, finite).
     * @param givenMinDistanceSamples Minimum sample separation between consecutive detected peaks (>= 1).
     * @param givenDetectValleys If true, detects both local maxima and local minima (valleys); if false, only maxima.
     * @param givenBufferSelector Buffer to analyze: "processed" or "raw".
     * @throws std::invalid_argument if base invariants fail, thresholds are negative/non-finite, or minDistanceSamples < 1.
     */
    PeakDetector(const std::string& givenName,
                 double givenMinAmplitude,
                 double givenMinProminence,
                 int givenMinDistanceSamples = 1,
                 bool givenDetectValleys = false,
                 const std::string& givenBufferSelector = "processed");

    /**
     * @brief Scans the signal for local extrema (including flat-top plateaus) and suppresses peaks within minDistanceSamples,
     *        retaining the highest-prominence extremum in each refractory cluster.
     *        Normalizes severity by baseline stdDev (with fallback divisor 1.0 when stdDev < 1e-12).
     * @param signal Read-only ChannelSignal to inspect.
     * @return EventRegistry populated with detected PeakEvent instances.
     */
    EventRegistry detect(const ChannelSignal& signal) const override;

    EventDetector* clone() const override;
    std::string getDescription() const override;

    double getMinAmplitude() const;
    double getMinProminence() const;
    int getMinDistanceSamples() const;
    bool isDetectingValleys() const;
};

#endif