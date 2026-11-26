#ifndef EVENT_REGISTRY_H
#define EVENT_REGISTRY_H

#include "DetectecEvent.h"

#include <ostream>
#include <string>

class EventSorter;

/**
 * @class EventRegistry
 * @brief Manual RAII polymorphic container owning a heterogeneous dynamic array of DetectedEvent instances
 *        without STL containers or smart pointers. Implements the complete Rule of Three with strong exception safety.
 *        Grants encapsulated friend access strictly to EventSorter so sorting algorithms operate in-place
 *        on the internal pointer array without exposing raw pointers publicly.
 * @invariant 0 <= count && count <= capacity.
 * @invariant If capacity == 0, then events == nullptr and count == 0.
 * @invariant For every 0 <= i < count, *(events + i) != nullptr.
 * @invariant No pointer address in [events, events + count) is duplicated or shared with another EventRegistry.
 */
class EventRegistry {
    friend class EventSorter;

private:
    DetectedEvent** events;
    int count;
    int capacity;

    /**
     * @brief Reallocates the internal pointer buffer to newCapacity without cloning existing events.
     * @param newCapacity Target buffer capacity (must be > 0 and >= count).
     */
    void resize(int newCapacity);

    /**
     * @brief Deletes every owned DetectedEvent and the backing pointer array, resetting the registry to empty (capacity 0).
     */
    void releaseResources();

public:
    /**
     * @brief Constructs an empty EventRegistry with the requested initial capacity.
     * @param initialCapacity Initial pointer array capacity (>= 0; 0 creates a null buffer that grows on first insertion).
     * @throws std::invalid_argument if initialCapacity < 0.
     */
    explicit EventRegistry(int initialCapacity = 8);

    /**
     * @brief Deep-copy constructor (Rule of Three) cloning every owned event polymorphically.
     *        Exception-safe: frees all previously cloned events if any clone() call throws.
     * @param other Source registry to duplicate.
     */
    EventRegistry(const EventRegistry& other);

    /**
     * @brief Copy-assignment operator (Rule of Three) providing the strong exception guarantee.
     *        Safely handles self-assignment and builds the complete cloned buffer before releasing existing resources.
     * @param other Source registry to assign from.
     * @return Reference to *this.
     */
    EventRegistry& operator=(const EventRegistry& other);

    /**
     * @brief Destructor (Rule of Three) releasing all owned DetectedEvent instances and the pointer array.
     */
    ~EventRegistry();

    /**
     * @brief Clones the given event polymorphically and stores the copy in the registry.
     * @param event Polymorphic reference to the event to copy.
     */
    void add(const DetectedEvent& event);

    /**
     * @brief Transfers exclusive heap ownership of an already-allocated DetectedEvent pointer into the registry.
     *        If buffer reallocation fails, adopt() deletes the passed pointer before rethrowing to prevent caller leaks.
     * @param event Non-null heap-allocated pointer not already owned by this registry.
     * @throws std::invalid_argument if event == nullptr or if the same address is already stored in this registry.
     */
    void adopt(DetectedEvent* event);

    /**
     * @brief Deletes all owned events and resets count to 0 while preserving allocated buffer capacity.
     */
    void clear();

    /**
     * @brief Stably sorts owned events in chronological order via EventSorter and ChronologicalComparator.
     */
    void sortChronologically();

    /**
     * @brief Stably sorts owned events by descending normalized severity via EventSorter and SeverityComparator.
     */
    void sortBySeverity();

    int getCount() const;
    int getCapacity() const;
    bool isEmpty() const;

    /**
     * @brief Returns a const reference to the event at the specified index with bounds checking.
     * @param index Zero-based index in [0, count - 1].
     * @return Const reference to the stored event.
     * @throws std::out_of_range if index < 0 or index >= count.
     */
    const DetectedEvent& get(int index) const;

    /**
     * @brief Counts how many stored events match the specified canonical type label (e.g., "PEAK").
     * @param type Event type string to match against DetectedEvent::getType().
     * @return Number of matching events (>= 0).
     */
    int countByType(const std::string& type) const;

    /**
     * @brief Builds and returns a new independent registry containing deep copies of all events matching type.
     * @param type Canonical event type identifier.
     * @return New EventRegistry owning clones of matching events.
     */
    EventRegistry filterByType(const std::string& type) const;

    /**
     * @brief Builds and returns a new independent registry containing deep copies of all events overlapping [startTime, endTime].
     *        An event overlaps when endTimeInSecs >= startTime && startTimeInSecs <= endTime.
     * @param startTime Lower time bound in seconds (finite).
     * @param endTime Upper time bound in seconds (finite, >= startTime).
     * @return New EventRegistry owning clones of overlapping events.
     * @throws std::invalid_argument if either bound is non-finite or startTime > endTime.
     */
    EventRegistry filterByTimeRange(double startTime, double endTime) const;

    /**
     * @brief Bounds-checked read-only indexed access to stored events.
     * @param index Zero-based index in [0, count - 1].
     * @throws std::out_of_range if index is out of bounds.
     */
    const DetectedEvent& operator[](int index) const;

    /**
     * @brief Bounds-checked mutable indexed access to stored events.
     * @param index Zero-based index in [0, count - 1].
     * @throws std::out_of_range if index is out of bounds.
     */
    DetectedEvent& operator[](int index);

    /**
     * @brief Clones and appends a single event to this registry.
     * @param event Event to clone and append.
     * @return Reference to *this.
     */
    EventRegistry& operator+=(const DetectedEvent& event);

    /**
     * @brief Deep-clones and appends all events from other into this registry.
     *        Safe under self-concatenation (reg += reg) and provides the strong exception guarantee.
     * @param other Source registry whose events will be cloned and appended.
     * @return Reference to *this.
     */
    EventRegistry& operator+=(const EventRegistry& other);

    /**
     * @brief Returns a new EventRegistry containing deep copies of *this followed by deep copies of other.
     *        Does not modify either operand.
     * @param other Right-hand registry to combine with *this.
     * @return Newly constructed combined EventRegistry.
     */
    EventRegistry operator+(const EventRegistry& other) const;

    /**
     * @brief Checks element-by-element semantic equality in sequence order using DetectedEvent::operator==.
     * @param other Registry to compare against.
     * @return true if both registries have the same count and equal events at every index.
     */
    bool operator==(const EventRegistry& other) const;

    /**
     * @brief Logical negation of operator==.
     * @param other Registry to compare against.
     * @return true if the registries differ in count or in any stored event.
     */
    bool operator!=(const EventRegistry& other) const;

    /**
     * @brief Formats and writes a summary of all stored events to the output stream via polymorphic dispatch.
     * @param os Destination output stream.
     * @param registry Registry to serialize.
     * @return Reference to os for stream chaining.
     */
    friend std::ostream& operator<<(std::ostream& os, const EventRegistry& registry);
};

#endif
