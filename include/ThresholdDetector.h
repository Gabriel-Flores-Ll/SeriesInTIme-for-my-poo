#ifndef THRESHOLD_DETECTOR_H
#define THRESHOLD_DETECTOR_H

#include "EventDetector.h"
#include "ThresholdEvent.h"

#include <string>

/**
 * @class ThresholdDetector
 * @brief Concrete detector identifying sustained threshold excursions using a Schmitt-trigger
 *        hysteresis state machine (separate activation and release thresholds) and minimum duration filtering.
 *        Follows the Rule of Zero.
 * @invariant std::isfinite(activationThreshold) and std::isfinite(releaseThreshold).
 * @invariant For UPPER crossings: releaseThreshold <= activationThreshold.
 * @invariant For LOWER crossings: releaseThreshold >= activationThreshold.
 * @invariant minDurationSamples >= 1.
 */
class ThresholdDetector : public EventDetector {
private:
    double activationThreshold;
    double releaseThreshold;
    int minDurationSamples;
    ThresholdEvent::CrossingType crossingType;

public:
    /**
     * @brief Constructs a validated Schmitt-trigger ThresholdDetector.
     * @param givenName Non-empty detector identifier.
     * @param givenActivationThreshold Threshold that triggers entry into an excursion (finite).
     * @param givenReleaseThreshold Threshold that ends an active excursion (finite; respects hysteresis order).
     * @param givenMinDurationSamples Minimum consecutive samples required to emit an event (>= 1).
     * @param givenCrossingType Direction of excursion (UPPER or LOWER).
     * @param givenBufferSelector Buffer to analyze: "processed" or "raw".
     * @throws std::invalid_argument if thresholds are non-finite, violate hysteresis direction,
     *         crossingType is invalid, or minDurationSamples < 1.
     */
    ThresholdDetector(const std::string& givenName,
                      double givenActivationThreshold,
                      double givenReleaseThreshold,
                      int givenMinDurationSamples = 1,
                      ThresholdEvent::CrossingType givenCrossingType = ThresholdEvent::CrossingType::UPPER,
                      const std::string& givenBufferSelector = "processed");

    /**
     * @brief Scans the signal with hysteresis and emits a ThresholdEvent for each excursion lasting
     *        at least minDurationSamples (including excursions still active at the final sample).
     *        Normalizes severity as |extremeValue - activationThreshold| / stdDev (fallback divisor 1.0 when stdDev < 1e-12).
     * @param signal Read-only ChannelSignal to inspect.
     * @return EventRegistry populated with detected ThresholdEvent instances.
     */
    EventRegistry detect(const ChannelSignal& signal) const override;

    EventDetector* clone() const override;
    std::string getDescription() const override;

    double getActivationThreshold() const;
    double getReleaseThreshold() const;
    int getMinDurationSamples() const;
    ThresholdEvent::CrossingType getCrossingType() const;
};

#endif