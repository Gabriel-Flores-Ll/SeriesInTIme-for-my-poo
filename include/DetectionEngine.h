#ifndef DETECTION_ENGINE_H
#define DETECTION_ENGINE_H

#include "ChannelSignal.h"
#include "EventDetector.h"
#include "EventRegistry.h"

#include <ostream>

/**
 * @class DetectionEngine
 * @brief Manual RAII polymorphic coordinator owning a dynamic array of EventDetector strategies
 *        (architecturally symmetric to Phase 2's ProcessingPipeline).
 *        Executes all registered detectors over single or multiple ChannelSignal channels and consolidates results.
 * @invariant 0 <= count && count <= capacity.
 * @invariant If capacity == 0, detectors == nullptr and count == 0.
 * @invariant For every 0 <= i < count, *(detectors + i) != nullptr and all addresses are unique.
 */
class DetectionEngine {
private:
    EventDetector** detectors;
    int count;
    int capacity;

    void resize(int newCapacity);
    void releaseResources();

public:
    /**
     * @brief Constructs an empty DetectionEngine with the requested initial capacity.
     * @param initialCapacity Initial pointer array capacity (>= 0).
     * @throws std::invalid_argument if initialCapacity < 0.
     */
    explicit DetectionEngine(int initialCapacity = 4);

    /**
     * @brief Deep-copy constructor (Rule of Three) cloning all owned detectors with strong exception safety.
     * @param other Source engine to duplicate.
     */
    DetectionEngine(const DetectionEngine& other);

    /**
     * @brief Copy-assignment operator (Rule of Three) providing the strong exception guarantee.
     * @param other Source engine to assign from.
     * @return Reference to *this.
     */
    DetectionEngine& operator=(const DetectionEngine& other);

    /**
     * @brief Destructor (Rule of Three) releasing all owned detectors and the backing pointer array.
     */
    ~DetectionEngine();

    /**
     * @brief Polymorphically clones detector and appends the copy to this engine.
     * @param detector Detector strategy to clone.
     */
    void addDetector(const EventDetector& detector);

    /**
     * @brief Transfers exclusive heap ownership of an already-allocated EventDetector into this engine.
     *        Deletes detector before rethrowing if internal reallocation fails.
     * @param detector Non-null heap-allocated detector pointer not already owned by this engine.
     * @throws std::invalid_argument if detector == nullptr or already owned by this engine.
     */
    void adoptDetector(EventDetector* detector);

    /**
     * @brief Deletes all registered detectors while preserving allocated buffer capacity.
     */
    void clearDetectors();

    int getDetectorCount() const;
    int getCapacity() const;
    bool isEmpty() const;

    /**
     * @brief Returns a bounds-checked read-only reference to the detector at index.
     * @param index Zero-based index in [0, count - 1].
     * @throws std::out_of_range if index is out of bounds.
     */
    const EventDetector& getDetector(int index) const;

    /**
     * @brief Runs all registered detectors on signal and returns a consolidated sorted EventRegistry.
     * @param signal Read-only target ChannelSignal.
     * @param sortBySeverityOrder If true, sorts by descending normalized severity; if false, chronologically.
     * @return Consolidated EventRegistry owning all detected events.
     */
    EventRegistry run(const ChannelSignal& signal, bool sortBySeverityOrder = false) const;

    /**
     * @brief Runs all registered detectors across an array of ChannelSignal pointers and consolidates all events.
     * @param signals Array of non-null const ChannelSignal pointers (may be nullptr only if signalCount == 0).
     * @param signalCount Number of channel pointers in signals (>= 0).
     * @param sortBySeverityOrder If true, sorts by descending normalized severity; if false, chronologically.
     * @return Unified EventRegistry owning all detected events across all channels.
     * @throws std::invalid_argument if signalCount < 0, signals == nullptr with signalCount > 0, or any entry is nullptr.
     */
    EventRegistry runMultiple(const ChannelSignal* const* signals,
                              int signalCount,
                              bool sortBySeverityOrder = false) const;

    /**
     * @brief Writes a summary listing every registered detector to the output stream.
     * @param os Destination output stream.
     * @param engine DetectionEngine to serialize.
     * @return Reference to os for chaining.
     */
    friend std::ostream& operator<<(std::ostream& os, const DetectionEngine& engine);
};

#endif