#ifndef STATISTICAL_ANOMALY_EVENT_H
#define STATISTICAL_ANOMALY_EVENT_H

#include "DetectecEvent.h"

#include <ostream>
#include <string>

/**
 * @class StatisticalAnomalyEvent
 * @brief Concrete detected event representing a statistically anomalous signal episode
 *        identified via sliding-window Z-score and/or normalized energy density ratio.
 *        Follows the Rule of Zero (owns no raw pointers).
 * @invariant std::isfinite(maxZScore), std::isfinite(energyRatio),
 *            std::isfinite(baselineMean), and std::isfinite(baselineStdDev).
 * @invariant baselineStdDev >= 0.0 and energyRatio >= 0.0.
 * @invariant involvedWindows >= 1.
 */
class StatisticalAnomalyEvent : public DetectedEvent {
private:
    double maxZScore;
    double energyRatio;
    double baselineMean;
    double baselineStdDev;
    int involvedWindows;

public:
    /**
     * @brief Constructs a validated StatisticalAnomalyEvent covering one or more merged sliding windows.
     * @param givenCode Non-empty channel code copied from MetadataSensor::getCodeOfChannel().
     * @param givenUnit Non-empty physical unit copied from MetadataSensor::getUnitOfMeasure().
     * @param givenInitIndex Starting sample index of the anomalous episode (>= 0).
     * @param givenEndIndex Ending sample index of the anomalous episode (>= givenInitIndex).
     * @param givenStartTimeInSecs Start timestamp in seconds (>= 0.0, finite).
     * @param givenEndTimeInSecs End timestamp in seconds (>= givenStartTimeInSecs, finite).
     * @param givenSeverity Non-negative finite severity score (>= 0.0).
     * @param givenMaxZScore Signed Z-score with maximum absolute magnitude observed in the episode (finite).
     * @param givenEnergyRatio Maximum normalized window energy ratio R_E(w) observed (>= 0.0, finite).
     * @param givenBaselineMean Global signal baseline mean mu (finite).
     * @param givenBaselineStdDev Global signal baseline Bessel standard deviation s (>= 0.0, finite).
     * @param givenInvolvedWindows Number of consecutive/overlapping windows merged into this event (>= 1).
     * @throws std::invalid_argument if any base invariant fails, any metric is non-finite,
     *         baselineStdDev < 0.0, energyRatio < 0.0, or involvedWindows < 1.
     */
    StatisticalAnomalyEvent(const std::string& givenCode,
                            const std::string& givenUnit,
                            int givenInitIndex,
                            int givenEndIndex,
                            double givenStartTimeInSecs,
                            double givenEndTimeInSecs,
                            double givenSeverity,
                            double givenMaxZScore,
                            double givenEnergyRatio,
                            double givenBaselineMean,
                            double givenBaselineStdDev,
                            int givenInvolvedWindows);

    /**
     * @brief Allocates and returns a polymorphic deep copy of this StatisticalAnomalyEvent.
     * @return Newly heap-allocated StatisticalAnomalyEvent* returned as DetectedEvent*.
     */
    DetectedEvent* clone() const override;

    /**
     * @brief Returns the canonical type identifier for statistical anomaly events.
     * @return Constant string literal "STATISTICAL_ANOMALY".
     */
    std::string getType() const override;

    /**
     * @brief Returns the primary quantitative metric of this event, defined as maxZScore.
     * @return Finite signed maximum-magnitude Z-score.
     */
    double getCharacteristicValue() const override;

    /**
     * @brief Writes a formatted summary of the statistical anomaly episode to the output stream.
     * @param os Destination output stream.
     */
    void print(std::ostream& os) const override;

    /**
     * @brief Returns the signed Z-score with greatest absolute magnitude in the episode.
     * @return Finite Z-score value.
     */
    double getMaxZScore() const;

    /**
     * @brief Returns the maximum normalized energy ratio R_E(w) observed in the episode.
     * @return Non-negative finite dimensionless energy ratio.
     */
    double getEnergyRatio() const;

    /**
     * @brief Returns the baseline mean mu of the analyzed channel.
     * @return Finite baseline mean in physical units.
     */
    double getBaselineMean() const;

    /**
     * @brief Returns the baseline standard deviation s of the analyzed channel.
     * @return Non-negative finite standard deviation in physical units.
     */
    double getBaselineStdDev() const;

    /**
     * @brief Returns how many anomalous windows were consolidated into this event.
     * @return Positive integer count (>= 1).
     */
    int getInvolvedWindows() const;
};

#endif