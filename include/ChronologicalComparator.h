#ifndef CHRONOLOGICAL_COMPARATOR_H
#define CHRONOLOGICAL_COMPARATOR_H

#include "EventComparator.h"

#include <string>

/**
 * @class ChronologicalComparator
 * @brief Concrete Functor ordering events chronologically by delegating directly to
 *        DetectedEvent::operator< (ascending by default, or reversed when descending).
 *        Follows the Rule of Zero.
 */
class ChronologicalComparator : public EventComparator {
private:
    bool ascending;

public:
    /**
     * @brief Constructs a chronological comparator.
     * @param sortAscending If true, earliest events come first; if false, latest first.
     */
    explicit ChronologicalComparator(bool sortAscending = true);

    bool isAscending() const;
    bool operator()(const DetectedEvent& first, const DetectedEvent& second) const override;
    std::string getName() const override;
};

#endif
