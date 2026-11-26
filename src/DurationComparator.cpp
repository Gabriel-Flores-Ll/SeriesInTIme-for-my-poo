#include "DurationComparator.h"

#include "DetectecEvent.h"

DurationComparator::DurationComparator(bool sortDescending)
    : descending(sortDescending) {}

bool DurationComparator::isDescending() const {
    return descending;
}

bool DurationComparator::operator()(const DetectedEvent& first, const DetectedEvent& second) const {
    const double durA = first.getDurationInSecs();
    const double durB = second.getDurationInSecs();
    if (durA != durB) {
        return descending ? (durA > durB) : (durA < durB);
    }

    const int samplesA = first.getDurationInSamples();
    const int samplesB = second.getDurationInSamples();
    if (samplesA != samplesB) {
        return descending ? (samplesA > samplesB) : (samplesA < samplesB);
    }

    return first < second;
}

std::string DurationComparator::getName() const {
    return descending ? "DurationComparator(DESC)" : "DurationComparator(ASC)";
}
