#ifndef EVENT_SORTER_H
#define EVENT_SORTER_H

class DetectedEvent;
class EventComparator;
class EventRegistry;

/**
 * @class EventSorter
 * @brief Non-instantiable OOP sorting utility (Unit 7) operating directly on EventRegistry's
 *        encapsulated polymorphic DetectedEvent** array via friend access (never exposing raw pointers).
 *        Provides O(n log n) Quicksort using relational operator overloading (DetectedEvent::operator<)
 *        or a polymorphic EventComparator Functor, and stable O(n^2) Insertion Sort parameterized
 *        with EventComparator Functors. All algorithms swap strictly DetectedEvent* pointers in O(1)
 *        using pointer arithmetic, guaranteeing zero heap allocations and zero Object Slicing.
 */
class EventSorter {
private:
    static void swapPointers(DetectedEvent** first, DetectedEvent** second);

    static int medianOfThreeRelational(DetectedEvent** arr, int low, int high);
    static void quickSortRelationalRange(DetectedEvent** arr, int low, int high);

    static int medianOfThreeComparator(DetectedEvent** arr,
                                       int low,
                                       int high,
                                       const EventComparator& comparator);
    static void quickSortComparatorRange(DetectedEvent** arr,
                                         int low,
                                         int high,
                                         const EventComparator& comparator);

public:
    EventSorter() = delete;

    /**
     * @brief Sorts registry in-place using O(n log n) Quicksort driven directly by
     *        DetectedEvent::operator< with median-of-three pivot selection and tail-call elimination.
     *        Exchanges only DetectedEvent* addresses in O(1) (no cloning, no Object Slicing).
     * @param registry Target EventRegistry to sort chronologically.
     */
    static void quickSort(EventRegistry& registry);

    /**
     * @brief Sorts registry in-place using O(n log n) Quicksort parameterized with a
     *        polymorphic EventComparator Functor, median-of-three pivoting, and O(1) pointer swaps.
     * @param registry Target EventRegistry to sort.
     * @param comparator Strict weak ordering Functor defining precedence.
     */
    static void quickSort(EventRegistry& registry, const EventComparator& comparator);

    /**
     * @brief Stably sorts registry in-place using O(n^2) Insertion Sort parameterized with a
     *        polymorphic EventComparator Functor and O(1) pointer shifts via pointer arithmetic.
     * @param registry Target EventRegistry to sort.
     * @param comparator Strict weak ordering Functor defining precedence.
     */
    static void insertionSort(EventRegistry& registry, const EventComparator& comparator);

    /**
     * @brief Verifies in O(n) whether registry is ordered according to DetectedEvent::operator<.
     * @param registry Read-only EventRegistry to inspect.
     * @return true if no adjacent pair violates chronological order (always true for count <= 1).
     */
    static bool isSorted(const EventRegistry& registry);

    /**
     * @brief Verifies in O(n) whether registry is ordered according to comparator.
     * @param registry Read-only EventRegistry to inspect.
     * @param comparator Strict weak ordering Functor to verify against.
     * @return true if no adjacent pair violates comparator (always true for count <= 1).
     */
    static bool isSorted(const EventRegistry& registry, const EventComparator& comparator);
};

#endif
