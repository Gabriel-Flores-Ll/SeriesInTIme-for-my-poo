#ifndef THRESHOLD_EVENT_H
#define THRESHOLD_EVENT_H

#include "DetectecEvent.h"

#include <ostream>
#include <string>

/**
 * @class ThresholdEvent
 * @brief Concrete detected event representing a sustained signal excursion beyond a threshold
 *        (upper or lower crossing) over a closed sample interval [initIndex, endIndex].
 *        Follows the Rule of Zero (owns no raw pointers).
 * @invariant std::isfinite(referenceThreshold) and std::isfinite(extremeValue).
 * @invariant If crossingType == CrossingType::UPPER, extremeValue >= referenceThreshold.
 * @invariant If crossingType == CrossingType::LOWER, extremeValue <= referenceThreshold.
 */
class ThresholdEvent : public DetectedEvent {
public:
    /**
     * @enum CrossingType
     * @brief Direction of the threshold excursion.
     */
    enum class CrossingType {
        UPPER,
        LOWER
    };

private:
    double referenceThreshold;
    double extremeValue;
    CrossingType crossingType;

public:
    /**
     * @brief Constructs a validated ThresholdEvent over an inclusive sample interval.
     * @param givenCode Non-empty channel code copied from MetadataSensor::getCodeOfChannel().
     * @param givenUnit Non-empty physical unit copied from MetadataSensor::getUnitOfMeasure().
     * @param givenInitIndex Starting sample index of the excursion (>= 0).
     * @param givenEndIndex Ending sample index of the excursion (>= givenInitIndex).
     * @param givenStartTimeInSecs Start timestamp in seconds (>= 0.0, finite).
     * @param givenEndTimeInSecs End timestamp in seconds (>= givenStartTimeInSecs, finite).
     * @param givenSeverity Non-negative finite severity score (>= 0.0).
     * @param givenReferenceThreshold Activation threshold crossed by the signal (finite).
     * @param givenExtremeValue Peak maximum (for UPPER) or minimum (for LOWER) reached during the excursion (finite).
     * @param givenCrossingType Direction of the excursion (UPPER or LOWER).
     * @throws std::invalid_argument if any base invariant fails, thresholds/extremes are non-finite,
     *         or extremeValue violates the crossing direction relative to referenceThreshold.
     */
    ThresholdEvent(const std::string& givenCode,
                   const std::string& givenUnit,
                   int givenInitIndex,
                   int givenEndIndex,
                   double givenStartTimeInSecs,
                   double givenEndTimeInSecs,
                   double givenSeverity,
                   double givenReferenceThreshold,
                   double givenExtremeValue,
                   CrossingType givenCrossingType);

    /**
     * @brief Allocates and returns a polymorphic deep copy of this ThresholdEvent.
     * @return Newly heap-allocated ThresholdEvent* returned as DetectedEvent*.
     */
    DetectedEvent* clone() const override;

    /**
     * @brief Returns the canonical type identifier for threshold events.
     * @return Constant string literal "THRESHOLD".
     */
    std::string getType() const override;

    /**
     * @brief Returns the primary quantitative metric of this event, defined as extremeValue.
     * @return Finite extreme amplitude reached during the excursion.
     */
    double getCharacteristicValue() const override;

    /**
     * @brief Writes a formatted summary of the threshold excursion to the output stream.
     * @param os Destination output stream.
     */
    void print(std::ostream& os) const override;

    /**
     * @brief Returns the activation threshold associated with this excursion.
     * @return Finite reference threshold in physical units.
     */
    double getReferenceThreshold() const;

    /**
     * @brief Returns the extreme value recorded within the excursion interval.
     * @return Finite maximum (UPPER) or minimum (LOWER) signal value.
     */
    double getExtremeValue() const;

    /**
     * @brief Returns the crossing direction of the threshold event.
     * @return CrossingType::UPPER or CrossingType::LOWER.
     */
    CrossingType getCrossingType() const;
};

#endif