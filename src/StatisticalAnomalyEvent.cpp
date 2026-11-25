#include "StatisticalAnomalyEvent.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

StatisticalAnomalyEvent::StatisticalAnomalyEvent(const std::string& givenCode,
                                                 const std::string& givenUnit,
                                                 int givenInitIndex,
                                                 int givenEndIndex,
                                                 double givenStartTimeInSecs,
                                                 double givenEndTimeInSecs,
                                                 double givenSeverity,
                                                 double givenMaxZScore,
                                                 double givenEnergyRatio,
                                                 double givenBaselineMean,
                                                 double givenBaselineStdDev,
                                                 int givenInvolvedWindows)
    : DetectedEvent(givenCode,
                    givenUnit,
                    givenInitIndex,
                    givenEndIndex,
                    givenStartTimeInSecs,
                    givenEndTimeInSecs,
                    givenSeverity),
      maxZScore(givenMaxZScore),
      energyRatio(givenEnergyRatio),
      baselineMean(givenBaselineMean),
      baselineStdDev(givenBaselineStdDev),
      involvedWindows(givenInvolvedWindows) {
    if (!std::isfinite(maxZScore) || !std::isfinite(energyRatio) ||
        !std::isfinite(baselineMean) || !std::isfinite(baselineStdDev)) {
        throw std::invalid_argument("StatisticalAnomalyEvent: all floating-point metrics must be finite.");
    }
    if (baselineStdDev < 0.0) {
        throw std::invalid_argument("StatisticalAnomalyEvent: baselineStdDev cannot be negative.");
    }
    if (energyRatio < 0.0) {
        throw std::invalid_argument("StatisticalAnomalyEvent: energyRatio cannot be negative.");
    }
    if (involvedWindows < 1) {
        throw std::invalid_argument("StatisticalAnomalyEvent: involvedWindows must be >= 1.");
    }
}

DetectedEvent* StatisticalAnomalyEvent::clone() const {
    return new StatisticalAnomalyEvent(*this);
}

std::string StatisticalAnomalyEvent::getType() const {
    return "STATISTICAL_ANOMALY";
}

double StatisticalAnomalyEvent::getCharacteristicValue() const {
    return maxZScore;
}

void StatisticalAnomalyEvent::print(std::ostream& os) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << "[" << getType() << "] "
        << "channel=" << getCodeOfChannel()
        << " | idx=[" << getInitIndex() << ", " << getEndIndex() << "]"
        << " | samples=" << getDurationInSamples()
        << " | t=[" << getStartTimeInSecs() << "s, " << getEndTimeInSecs() << "s]"
        << " | maxZ=" << maxZScore
        << " | energyRatio=" << energyRatio
        << " | baseline=(" << baselineMean << ", sd=" << baselineStdDev << ") " << getUnitOfMeasure()
        << " | windows=" << involvedWindows
        << " | severity=" << getSeverity();
    os << oss.str();
}

double StatisticalAnomalyEvent::getMaxZScore() const {
    return maxZScore;
}

double StatisticalAnomalyEvent::getEnergyRatio() const {
    return energyRatio;
}

double StatisticalAnomalyEvent::getBaselineMean() const {
    return baselineMean;
}

double StatisticalAnomalyEvent::getBaselineStdDev() const {
    return baselineStdDev;
}

int StatisticalAnomalyEvent::getInvolvedWindows() const {
    return involvedWindows;
}