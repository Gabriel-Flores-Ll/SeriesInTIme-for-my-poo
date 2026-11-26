#include "ChronologicalComparator.h"

#include "DetectecEvent.h"

ChronologicalComparator::ChronologicalComparator(bool sortAscending)
    : ascending(sortAscending) {}

bool ChronologicalComparator::isAscending() const {
    return ascending;
}

bool ChronologicalComparator::operator()(const DetectedEvent& first, const DetectedEvent& second) const {
    return ascending ? (first < second) : (second < first);
}

std::string ChronologicalComparator::getName() const {
    return ascending ? "ChronologicalComparator(ASC)" : "ChronologicalComparator(DESC)";
}
