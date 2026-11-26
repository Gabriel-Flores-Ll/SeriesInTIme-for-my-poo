#ifndef DURATION_COMPARATOR_H
#define DURATION_COMPARATOR_H

#include "EventComparator.h"

#include <string>

/**
 * @class DurationComparator
 * @brief Concrete Functor ordering events by physical duration in seconds
 *        (descending by default to prioritize sustained bursts over isolated spikes),
 *        breaking ties by sample span and then DetectedEvent::operator<.
 *        Follows the Rule of Zero.
 */
class DurationComparator : public EventComparator {
private:
    bool descending;

public:
    /**
     * @brief Constructs a duration comparator.
     * @param sortDescending If true, longest events come first; if false, shortest first.
     */
    explicit DurationComparator(bool sortDescending = true);

    bool isDescending() const;
    bool operator()(const DetectedEvent& first, const DetectedEvent& second) const override;
    std::string getName() const override;
};

#endif
