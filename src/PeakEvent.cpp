#include "PeakEvent.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

PeakEvent::PeakEvent(const std::string& givenCode,
                     const std::string& givenUnit,
                     int givenPeakIndex,
                     double givenPeakTimeInSecs,
                     double givenSeverity,
                     double givenPeakAmplitude,
                     double givenProminence,
                     bool givenIsPositive)
    : DetectedEvent(givenCode,
                    givenUnit,
                    givenPeakIndex,
                    givenPeakIndex,
                    givenPeakTimeInSecs,
                    givenPeakTimeInSecs,
                    givenSeverity),
      peakAmplitude(givenPeakAmplitude),
      prominence(givenProminence),
      positivePeak(givenIsPositive) {
    if (!std::isfinite(peakAmplitude)) {
        throw std::invalid_argument("PeakEvent: peakAmplitude must be finite.");
    }
    if (!std::isfinite(prominence) || prominence < 0.0) {
        throw std::invalid_argument("PeakEvent: prominence must be finite and non-negative.");
    }
}

DetectedEvent* PeakEvent::clone() const {
    return new PeakEvent(*this);
}

std::string PeakEvent::getType() const {
    return "PEAK";
}

double PeakEvent::getCharacteristicValue() const {
    return peakAmplitude;
}

void PeakEvent::print(std::ostream& os) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << "[" << getType() << "] "
        << "channel=" << getCodeOfChannel()
        << " | idx=" << getInitIndex()
        << " | t=" << getStartTimeInSecs() << "s"
        << " | polarity=" << (positivePeak ? "POSITIVE" : "NEGATIVE")
        << " | amplitude=" << peakAmplitude << " " << getUnitOfMeasure()
        << " | prominence=" << prominence << " " << getUnitOfMeasure()
        << " | severity=" << getSeverity();
    os << oss.str();
}

double PeakEvent::getPeakAmplitude() const {
    return peakAmplitude;
}

double PeakEvent::getProminence() const {
    return prominence;
}

bool PeakEvent::isPositivePeak() const {
    return positivePeak;
}