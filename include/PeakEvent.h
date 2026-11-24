#ifndef PEAK_EVENT_H
#define PEAK_EVENT_H

#include "DetectecEvent.h"

#include <ostream>
#include <string>

/**
 * @class PeakEvent
 * @brief Concrete detected event representing a single-sample local extremum (peak or valley) on a signal channel.
 *        Follows the Rule of Zero (owns no raw pointers) and models a degenerate closed interval [peakIndex, peakIndex]
 *        spanning exactly 1 sample and 0.0 seconds of elapsed span.
 * @invariant initIndex == endIndex and startTimeInSecs == endTimeInSecs (enforced via base constructor delegation).
 * @invariant std::isfinite(peakAmplitude).
 * @invariant prominence >= 0.0 and std::isfinite(prominence).
 */
class PeakEvent : public DetectedEvent {
private:
    double peakAmplitude;
    double prominence;
    bool positivePeak;

public:
    /**
     * @brief Constructs a validated single-sample PeakEvent by delegating shared metadata to DetectedEvent.
     *        Passes givenPeakIndex as both initIndex and endIndex, and givenPeakTimeInSecs as both start and end timestamps.
     * @param givenCode Non-empty channel code copied from MetadataSensor::getCodeOfChannel().
     * @param givenUnit Non-empty physical unit copied from MetadataSensor::getUnitOfMeasure().
     * @param givenPeakIndex Sample index of the detected peak or valley (>= 0).
     * @param givenPeakTimeInSecs Timestamp of the peak in seconds (>= 0.0, finite).
     * @param givenSeverity Non-negative finite severity score (>= 0.0).
     * @param givenPeakAmplitude Signal value at the detected extremum (finite; may be negative regardless of positivePeak).
     * @param givenProminence Non-negative finite magnitude of the peak relative to the signal baseline (>= 0.0).
     * @param givenIsPositive True for a local maximum (positive peak), false for a local minimum (valley).
     * @throws std::invalid_argument if any base DetectedEvent invariant is violated, or if givenPeakAmplitude is non-finite,
     *         or if givenProminence is negative or non-finite.
     */
    PeakEvent(const std::string& givenCode,
              const std::string& givenUnit,
              int givenPeakIndex,
              double givenPeakTimeInSecs,
              double givenSeverity,
              double givenPeakAmplitude,
              double givenProminence,
              bool givenIsPositive);

    /**
     * @brief Allocates and returns a polymorphic deep copy of this PeakEvent (caller/registry takes ownership).
     * @return Newly heap-allocated PeakEvent* returned as DetectedEvent*.
     */
    DetectedEvent* clone() const override;

    /**
     * @brief Returns the canonical type identifier for peak events.
     * @return Constant string literal "PEAK".
     */
    std::string getType() const override;

    /**
     * @brief Returns the primary quantitative metric of this event, defined as peakAmplitude.
     * @return Finite amplitude value of the peak or valley.
     */
    double getCharacteristicValue() const override;

    /**
     * @brief Writes a formatted summary of the peak event (channel, index, timestamp, polarity, amplitude, prominence, severity) to the stream.
     * @param os Destination output stream.
     */
    void print(std::ostream& os) const override;

    /**
     * @brief Returns the signal amplitude measured at the peak index.
     * @return Finite peak amplitude in the channel's physical unit.
     */
    double getPeakAmplitude() const;

    /**
     * @brief Returns the baseline-relative prominence of the peak.
     * @return Non-negative finite prominence value.
     */
    double getProminence() const;

    /**
     * @brief Indicates whether the detected extremum is a local maximum or a local minimum (valley).
     * @return true if local maximum (positive peak), false if local minimum (valley).
     */
    bool isPositivePeak() const;
};

#endif