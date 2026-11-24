#include "DetectecEvent.h"

#include <cmath>
#include <stdexcept>

namespace {
constexpr double kValueTolerance = 1e-9;
}

DetectedEvent::DetectedEvent(const std::string& givenCode,
                             const std::string& givenUnit,
                             int givenInitIndex,
                             int givenEndIndex,
                             double givenStartTimeInSecs,
                             double givenEndTimeInSecs,
                             double givenSeverity)
    : codeOfChannel(givenCode),
      unitOfMeasure(givenUnit),
      initIndex(givenInitIndex),
      endIndex(givenEndIndex),
      startTimeInSecs(givenStartTimeInSecs),
      endTimeInSecs(givenEndTimeInSecs),
      severity(givenSeverity) {
    if (codeOfChannel.empty()) {
        throw std::invalid_argument("DetectedEvent: codeOfChannel cannot be empty.");
    }
    if (unitOfMeasure.empty()) {
        throw std::invalid_argument("DetectedEvent: unitOfMeasure cannot be empty.");
    }
    if (initIndex < 0 || endIndex < initIndex) {
        throw std::invalid_argument("DetectedEvent: invalid sample index interval.");
    }
    if (!std::isfinite(startTimeInSecs) || !std::isfinite(endTimeInSecs) ||
        startTimeInSecs < 0.0 || endTimeInSecs < startTimeInSecs) {
        throw std::invalid_argument("DetectedEvent: invalid timestamp interval.");
    }
    if (!std::isfinite(severity) || severity < 0.0) {
        throw std::invalid_argument("DetectedEvent: severity must be finite and non-negative.");
    }
}

const std::string& DetectedEvent::getCodeOfChannel() const {
    return codeOfChannel;
}

const std::string& DetectedEvent::getUnitOfMeasure() const {
    return unitOfMeasure;
}

int DetectedEvent::getInitIndex() const {
    return initIndex;
}

int DetectedEvent::getEndIndex() const {
    return endIndex;
}

int DetectedEvent::getDurationInSamples() const {
    return endIndex - initIndex + 1;
}

double DetectedEvent::getStartTimeInSecs() const {
    return startTimeInSecs;
}

double DetectedEvent::getEndTimeInSecs() const {
    return endTimeInSecs;
}

double DetectedEvent::getDurationInSecs() const {
    return endTimeInSecs - startTimeInSecs;
}

double DetectedEvent::getSeverity() const {
    return severity;
}

bool DetectedEvent::operator<(const DetectedEvent& other) const {
    if (initIndex != other.initIndex) {
        return initIndex < other.initIndex;
    }
    if (endIndex != other.endIndex) {
        return endIndex < other.endIndex;
    }
    if (severity != other.severity) {
        return severity > other.severity;
    }

    const std::string myType = getType();
    const std::string otherType = other.getType();
    if (myType != otherType) {
        return myType < otherType;
    }

    return codeOfChannel < other.codeOfChannel;
}

bool DetectedEvent::operator==(const DetectedEvent& other) const {
    return getType() == other.getType() &&
           getCodeOfChannel() == other.getCodeOfChannel() &&
           getInitIndex() == other.getInitIndex() &&
           getEndIndex() == other.getEndIndex() &&
           std::fabs(getSeverity() - other.getSeverity()) < kValueTolerance &&
           std::fabs(getCharacteristicValue() - other.getCharacteristicValue()) < kValueTolerance;
}

bool DetectedEvent::operator!=(const DetectedEvent& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const DetectedEvent& event) {
    event.print(os);
    return os;
}