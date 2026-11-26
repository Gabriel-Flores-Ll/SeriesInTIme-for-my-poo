#ifndef SEVERITY_COMPARATOR_H
#define SEVERITY_COMPARATOR_H

#include "EventComparator.h"

#include <string>

/**
 * @class SeverityComparator
 * @brief Concrete Functor ordering events by normalized severity (descending by default
 *        for clinical triage), breaking ties deterministically via DetectedEvent::operator<.
 *        Follows the Rule of Zero.
 */
class SeverityComparator : public EventComparator {
private:
    bool descending;

public:
    /**
     * @brief Constructs a severity comparator.
     * @param sortDescending If true, highest severity comes first; if false, lowest first.
     */
    explicit SeverityComparator(bool sortDescending = true);

    bool isDescending() const;
    bool operator()(const DetectedEvent& first, const DetectedEvent& second) const override;
    std::string getName() const override;
};

#endif
