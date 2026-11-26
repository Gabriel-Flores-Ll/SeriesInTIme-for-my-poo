#ifndef DETECTED_EVENT_H
#define DETECTED_EVENT_H

#include <ostream>
#include <string>

/**
 * @class DetectedEvent
 * @brief Abstract base class representing a detected event or anomaly on a time-series signal channel.
 *        Stores channel metadata and timestamps by value to remain completely decoupled from ChannelSignal's lifetime.
 * @invariant !codeOfChannel.empty() and !unitOfMeasure.empty().
 * @invariant 0 <= initIndex && initIndex <= endIndex.
 * @invariant 0.0 <= startTimeInSecs && startTimeInSecs <= endTimeInSecs (both finite).
 * @invariant severity >= 0.0 and std::isfinite(severity).
 */
class DetectedEvent {
private:
    std::string codeOfChannel;
    std::string unitOfMeasure;
    int initIndex;
    int endIndex;
    double startTimeInSecs;
    double endTimeInSecs;
    double severity;

protected:
    /**
     * @brief Validates invariants and initializes the common state of any concrete detected event.
     * @param givenCode Non-empty channel code copied from MetadataSensor::getCodeOfChannel().
     * @param givenUnit Non-empty physical unit copied from MetadataSensor::getUnitOfMeasure().
     * @param givenInitIndex Starting sample index of the closed interval [initIndex, endIndex] (>= 0).
     * @param givenEndIndex Ending sample index of the closed interval (>= givenInitIndex).
     * @param givenStartTimeInSecs Start timestamp in seconds (>= 0.0, finite).
     * @param givenEndTimeInSecs End timestamp in seconds (>= givenStartTimeInSecs, finite).
     * @param givenSeverity Non-negative finite severity score (>= 0.0).
     * @throws std::invalid_argument if any class invariant is violated.
     */
    DetectedEvent(const std::string& givenCode,
                  const std::string& givenUnit,
                  int givenInitIndex,
                  int givenEndIndex,
                  double givenStartTimeInSecs,
                  double givenEndTimeInSecs,
                  double givenSeverity);

    /**
     * @brief Protected copy constructor preventing external object slicing while allowing derived clone() implementations.
     */
    DetectedEvent(const DetectedEvent& other) = default;

    /**
     * @brief Protected copy assignment operator preventing partial assignment through base references.
     */
    DetectedEvent& operator=(const DetectedEvent& other) = default;

public:
    /**
     * @brief Virtual destructor ensuring safe polymorphic destruction through base pointers (Rule of Zero).
     */
    virtual ~DetectedEvent() = default;

    /**
     * @brief Allocates and returns a polymorphic deep copy of the concrete event (caller/registry takes ownership).
     * @return Newly heap-allocated copy preserving the exact dynamic type.
     */
    virtual DetectedEvent* clone() const = 0;

    /**
     * @brief Returns the canonical string identifier of the concrete event type (e.g., "PEAK", "THRESHOLD", "STATISTICAL_ANOMALY").
     * @return Type label used for classification, comparison, and filtering.
     */
    virtual std::string getType() const = 0;

    /**
     * @brief Returns the primary quantitative metric characterizing the event (e.g., peak amplitude, extreme value, or max Z-score).
     * @return Finite representative metric of the concrete event.
     */
    virtual double getCharacteristicValue() const = 0;

    /**
     * @brief Formats and writes a human-readable representation of the concrete event to an output stream.
     * @param os Destination output stream.
     */
    virtual void print(std::ostream& os) const = 0;

    const std::string& getCodeOfChannel() const;
    const std::string& getUnitOfMeasure() const;
    int getInitIndex() const;
    int getEndIndex() const;

    /**
     * @brief Returns the inclusive sample span covered by the event (endIndex - initIndex + 1).
     * @return Positive sample count (>= 1).
     */
    int getDurationInSamples() const;

    double getStartTimeInSecs() const;
    double getEndTimeInSecs() const;

    /**
     * @brief Returns the elapsed time span between the first and last sample of the event (endTimeInSecs - startTimeInSecs).
     * @return Non-negative duration in seconds (0.0 for single-sample events).
     */
    double getDurationInSecs() const;

    double getSeverity() const;

    /**
     * @brief Strict weak chronological ordering for deterministic sorting in EventRegistry.
     *        Compares in order: initIndex (asc), endIndex (asc), severity (desc), getType() (asc), codeOfChannel (asc).
     * @param other Event to compare against.
     * @return true if *this strictly precedes other.
     */
    bool operator<(const DetectedEvent& other) const;

    /**
     * @brief Checks semantic equality across event type, channel code, sample interval, and characteristic value (tolerance 1e-9).
     * @param other Event to compare against.
     * @return true if both events represent the same detection.
     */
    bool operator==(const DetectedEvent& other) const;

    /**
     * @brief Logical negation of operator==.
     * @param other Event to compare against.
     * @return true if the events differ.
     */
    bool operator!=(const DetectedEvent& other) const;

    /**
     * @brief Polymorphic stream insertion operator delegating to virtual print() (Virtual Friend Function idiom).
     * @param os Destination output stream.
     * @param event Polymorphic reference to the event to print.
     * @return Reference to os for stream chaining.
     */
    friend std::ostream& operator<<(std::ostream& os, const DetectedEvent& event);
};

#endif