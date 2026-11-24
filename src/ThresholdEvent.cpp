#include "ThresholdEvent.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

ThresholdEvent::ThresholdEvent(const std::string& givenCode,
                               const std::string& givenUnit,
                               int givenInitIndex,
                               int givenEndIndex,
                               double givenStartTimeInSecs,
                               double givenEndTimeInSecs,
                               double givenSeverity,
                               double givenReferenceThreshold,
                               double givenExtremeValue,
                               CrossingType givenCrossingType)
    : DetectedEvent(givenCode,
                    givenUnit,
                    givenInitIndex,
                    givenEndIndex,
                    givenStartTimeInSecs,
                    givenEndTimeInSecs,
                    givenSeverity),
      referenceThreshold(givenReferenceThreshold),
      extremeValue(givenExtremeValue),
      crossingType(givenCrossingType) {
    if (!std::isfinite(referenceThreshold) || !std::isfinite(extremeValue)) {
        throw std::invalid_argument("ThresholdEvent: referenceThreshold and extremeValue must be finite.");
    }
    if (crossingType == CrossingType::UPPER) {
        if (extremeValue < referenceThreshold) {
            throw std::invalid_argument("ThresholdEvent: extremeValue must be >= referenceThreshold for UPPER crossing.");
        }
    } else if (crossingType == CrossingType::LOWER) {
        if (extremeValue > referenceThreshold) {
            throw std::invalid_argument("ThresholdEvent: extremeValue must be <= referenceThreshold for LOWER crossing.");
        }
    } else {
        throw std::invalid_argument("ThresholdEvent: invalid CrossingType.");
    }
}

DetectedEvent* ThresholdEvent::clone() const {
    return new ThresholdEvent(*this);
}

std::string ThresholdEvent::getType() const {
    return "THRESHOLD";
}

double ThresholdEvent::getCharacteristicValue() const {
    return extremeValue;
}

void ThresholdEvent::print(std::ostream& os) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << "[" << getType() << "] "
        << "channel=" << getCodeOfChannel()
        << " | idx=[" << getInitIndex() << ", " << getEndIndex() << "]"
        << " | samples=" << getDurationInSamples()
        << " | t=[" << getStartTimeInSecs() << "s, " << getEndTimeInSecs() << "s]"
        << " | crossing=" << (crossingType == CrossingType::UPPER ? "UPPER" : "LOWER")
        << " | threshold=" << referenceThreshold << " " << getUnitOfMeasure()
        << " | extreme=" << extremeValue << " " << getUnitOfMeasure()
        << " | severity=" << getSeverity();
    os << oss.str();
}

double ThresholdEvent::getReferenceThreshold() const {
    return referenceThreshold;
}

double ThresholdEvent::getExtremeValue() const {
    return extremeValue;
}

ThresholdEvent::CrossingType ThresholdEvent::getCrossingType() const {
    return crossingType;
}