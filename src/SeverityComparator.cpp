#include "SeverityComparator.h"

#include "DetectecEvent.h"

SeverityComparator::SeverityComparator(bool sortDescending)
    : descending(sortDescending) {}

bool SeverityComparator::isDescending() const {
    return descending;
}

bool SeverityComparator::operator()(const DetectedEvent& first, const DetectedEvent& second) const {
    const double sevA = first.getSeverity();
    const double sevB = second.getSeverity();
    if (sevA != sevB) {
        return descending ? (sevA > sevB) : (sevA < sevB);
    }
    return first < second;
}

std::string SeverityComparator::getName() const {
    return descending ? "SeverityComparator(DESC)" : "SeverityComparator(ASC)";
}
