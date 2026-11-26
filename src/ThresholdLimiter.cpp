#include "ThresholdLimiter.h"

#include <cmath>
#include <iomanip>
#include <sstream>

ThresholdLimiter::ThresholdLimiter(double givenMin,
                                   double givenMax,
                                   double givenScaleFactor)
    : PointTransformer("ThresholdLimiter", givenScaleFactor),
      lowerLimit(-1.0),
      upperLimit(1.0) {
    setLimits(givenMin, givenMax);
}

double ThresholdLimiter::getLowerLimit() const {
    return lowerLimit;
}

double ThresholdLimiter::getUpperLimit() const {
    return upperLimit;
}

bool ThresholdLimiter::setLimits(double givenMin, double givenMax) {
    if (!std::isfinite(givenMin) || !std::isfinite(givenMax) || givenMin >= givenMax) {
        return false;
    }
    lowerLimit = givenMin;
    upperLimit = givenMax;
    return true;
}

double ThresholdLimiter::transform(double givenSample) const {
    if (givenSample < lowerLimit) {
        return lowerLimit;
    }
    if (givenSample > upperLimit) {
        return upperLimit;
    }
    return givenSample;
}

ProcessingModule* ThresholdLimiter::clone() const {
    return new ThresholdLimiter(*this);
}

std::string ThresholdLimiter::serialize() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6)
        << "LIMITER;min=" << lowerLimit
        << ";max=" << upperLimit
        << ";scale=" << getScaleFactor()
        << ";active=" << (isActive() ? "1" : "0");
    return oss.str();
}
