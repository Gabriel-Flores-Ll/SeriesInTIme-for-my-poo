#ifndef WINDOW_ANOMALY_DETECTOR_H
#define WINDOW_ANOMALY_DETECTOR_H

#include "EventDetector.h"

#include <string>

/**
 * @class WindowAnomalyDetector
 * @brief Concrete detector identifying statistical anomalies over sliding windows using Z-score
 *        and normalized energy ratio R_E(w). Instantiates a zero-copy SlicingWindow locally on the stack
 *        during detect() (local dependency, zero heap overhead) and merges overlapping/contiguous anomalous windows.
 *        Follows the Rule of Zero.
 * @invariant windowSize >= 1 and stride >= 1.
 * @invariant zScoreThreshold > 0.0 and std::isfinite(zScoreThreshold).
 * @invariant energyRatioThreshold > 0.0 and std::isfinite(energyRatioThreshold).
 */
class WindowAnomalyDetector : public EventDetector {
private:
    int windowSize;
    int stride;
    double zScoreThreshold;
    double energyRatioThreshold;

public:
    /**
     * @brief Constructs a validated WindowAnomalyDetector.
     * @param givenName Non-empty detector identifier.
     * @param givenWindowSize Number of samples W per sliding window (>= 1).
     * @param givenStride Step size S in samples between consecutive windows (>= 1).
     * @param givenZScoreThreshold Minimum |Z_w| required to flag a window (> 0.0, finite).
     * @param givenEnergyRatioThreshold Minimum energy ratio R_E(w) required to flag a window (> 0.0, finite).
     * @param givenBufferSelector Buffer to analyze: "processed" or "raw".
     * @throws std::invalid_argument if windowSize < 1, stride < 1, or thresholds are non-positive/non-finite.
     */
    WindowAnomalyDetector(const std::string& givenName,
                          int givenWindowSize,
                          int givenStride,
                          double givenZScoreThreshold = 2.5,
                          double givenEnergyRatioThreshold = 2.5,
                          const std::string& givenBufferSelector = "processed");

    /**
     * @brief Evaluates sliding windows over signal using a stack-allocated SlicingWindow.
     *        Merges overlapping or contiguous anomalous windows into consolidated StatisticalAnomalyEvent episodes.
     *        Computes linear dimensionless severity as max(|maxZScore|, sqrt(maxEnergyRatio)).
     *        Returns an empty registry if signal.getNumberOfSamples() < windowSize.
     * @param signal Read-only ChannelSignal to inspect.
     * @return EventRegistry populated with consolidated StatisticalAnomalyEvent instances.
     */
    EventRegistry detect(const ChannelSignal& signal) const override;

    EventDetector* clone() const override;
    std::string getDescription() const override;

    int getWindowSize() const;
    int getStride() const;
    double getZScoreThreshold() const;
    double getEnergyRatioThreshold() const;
};

#endif