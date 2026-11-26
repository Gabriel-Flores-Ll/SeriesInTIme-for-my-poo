#ifndef EVENT_DETECTOR_H
#define EVENT_DETECTOR_H

#include "ChannelSignal.h"
#include "EventRegistry.h"

#include <ostream>
#include <string>

/**
 * @class EventDetector
 * @brief Abstract base class defining the Strategy interface for signal event and anomaly detectors.
 *        Does not own any ChannelSignal (uses it strictly by const reference during detect()).
 *        Protected copy operations prevent object slicing while enabling polymorphic clone() in subclasses (Rule of Zero).
 * @invariant !name.empty().
 * @invariant bufferSelector == "processed" || bufferSelector == "raw".
 */
class EventDetector {
private:
    std::string name;
    std::string bufferSelector;

protected:
    static constexpr double kBaselineEpsilon = 1e-12;

    EventDetector(const EventDetector& other) = default;
    EventDetector& operator=(const EventDetector& other) = default;

    /**
     * @brief Reads sample at index from the buffer indicated by bufferSelector ("processed" or "raw").
     * @param signal Source signal channel.
     * @param index Sample index in [0, signal.getNumberOfSamples()).
     * @return Sample value from the selected buffer.
     */
    double sampleAt(const ChannelSignal& signal, int index) const;

    /**
     * @brief Retrieves baseline mean, Bessel standard deviation, and total energy for the selected buffer.
     *        Uses ChannelSignal::getProcessedStatistics() for "processed", or a two-pass calculation for "raw".
     * @param signal Source signal channel.
     * @param outMean Output parameter receiving the baseline mean.
     * @param outStdDev Output parameter receiving the baseline Bessel standard deviation (>= 0.0).
     * @param outTotalEnergy Output parameter receiving the baseline total signal energy (>= 0.0).
     */
    void computeBaseline(const ChannelSignal& signal,
                         double& outMean,
                         double& outStdDev,
                         double& outTotalEnergy) const;

public:
    /**
     * @brief Constructs a validated EventDetector base subobject.
     * @param givenName Non-empty human-readable detector name.
     * @param givenBufferSelector Buffer to analyze: must be strictly "processed" or "raw".
     * @throws std::invalid_argument if givenName is empty or givenBufferSelector is neither "processed" nor "raw".
     */
    EventDetector(const std::string& givenName,
                  const std::string& givenBufferSelector = "processed");

    /**
     * @brief Virtual destructor enabling safe polymorphic deletion through base pointers.
     */
    virtual ~EventDetector() = default;

    /**
     * @brief Executes the concrete detection algorithm on signal and returns all detected events.
     * @param signal Read-only target ChannelSignal.
     * @return EventRegistry owning all events found during analysis.
     */
    virtual EventRegistry detect(const ChannelSignal& signal) const = 0;

    /**
     * @brief Allocates and returns a polymorphic deep copy of this detector.
     * @return Newly heap-allocated EventDetector* owned by the caller.
     */
    virtual EventDetector* clone() const = 0;

    /**
     * @brief Returns a formatted description summarizing the detector type, name, buffer, and parameters.
     * @return Descriptive configuration string.
     */
    virtual std::string getDescription() const = 0;

    const std::string& getName() const;
    const std::string& getBufferSelector() const;

    /**
     * @brief Writes the polymorphic detector description to the output stream.
     * @param os Destination output stream.
     * @param detector Detector to describe.
     * @return Reference to os for chaining.
     */
    friend std::ostream& operator<<(std::ostream& os, const EventDetector& detector);
};

#endif