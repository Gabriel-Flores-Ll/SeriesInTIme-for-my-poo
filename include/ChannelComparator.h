#ifndef CHANNEL_COMPARATOR_H
#define CHANNEL_COMPARATOR_H

#include "EventComparator.h"

#include <string>

/**
 * @class ChannelComparator
 * @brief Concrete Functor grouping multi-channel detections by channel code
 *        (alphabetical ascending by default) and ordering events chronologically
 *        within each channel via DetectedEvent::operator<.
 *        Follows the Rule of Zero.
 */
class ChannelComparator : public EventComparator {
private:
    bool ascending;

public:
    /**
     * @brief Constructs a channel-grouping comparator.
     * @param sortAscending If true, channels are ordered A->Z; if false, Z->A.
     */
    explicit ChannelComparator(bool sortAscending = true);

    bool isAscending() const;
    bool operator()(const DetectedEvent& first, const DetectedEvent& second) const override;
    std::string getName() const override;
};

#endif
