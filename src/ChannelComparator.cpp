#include "ChannelComparator.h"

#include "DetectecEvent.h"

ChannelComparator::ChannelComparator(bool sortAscending)
    : ascending(sortAscending) {}

bool ChannelComparator::isAscending() const {
    return ascending;
}

bool ChannelComparator::operator()(const DetectedEvent& first, const DetectedEvent& second) const {
    const std::string& chA = first.getCodeOfChannel();
    const std::string& chB = second.getCodeOfChannel();
    if (chA != chB) {
        return ascending ? (chA < chB) : (chA > chB);
    }
    return first < second;
}

std::string ChannelComparator::getName() const {
    return ascending ? "ChannelComparator(ASC)" : "ChannelComparator(DESC)";
}
