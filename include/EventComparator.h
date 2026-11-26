#ifndef EVENT_COMPARATOR_H
#define EVENT_COMPARATOR_H

#include <string>

class DetectedEvent;

/**
 * @class EventComparator
 * @brief Abstract base class (ABC) defining the polymorphic Functor interface for
 *        comparing DetectedEvent instances under a strict weak ordering.
 *        Decouples sorting criteria from sorting algorithms in EventSorter without templates.
 *        Protected copy operations prevent object slicing.
 */
class EventComparator {
protected:
    EventComparator() = default;
    EventComparator(const EventComparator& other) = default;
    EventComparator& operator=(const EventComparator& other) = default;

public:
    /**
     * @brief Virtual destructor enabling safe polymorphic cleanup through base pointers.
     */
    virtual ~EventComparator() = default;

    /**
     * @brief Strict weak ordering predicate invoked as a function object (Functor).
     * @param first Left-hand detected event.
     * @param second Right-hand detected event.
     * @return true if first must strictly precede second in the target order; false otherwise.
     */
    virtual bool operator()(const DetectedEvent& first, const DetectedEvent& second) const = 0;

    /**
     * @brief Returns a short canonical description of this comparison policy.
     */
    virtual std::string getName() const = 0;
};

#endif
