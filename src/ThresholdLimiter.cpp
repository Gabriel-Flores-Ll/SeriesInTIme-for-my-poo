#include "ThresholdLimiter.h"

ThresholdLimiter::ThresholdLimiter(double givenMin, double givenMax)
    : PointTransformer("ThresholdLimiter", 1.0), lowerLimit(-1.0), upperLimit(1.0) {
    setLimits(givenMin, givenMax);
}

double ThresholdLimiter::getLowerLimit() const {return lowerLimit;}
double ThresholdLimiter::getUpperLimit() const {return upperLimit;}

bool ThresholdLimiter::setLimits(double givenMin, double givenMax) {
    if (givenMin < givenMax) {
        lowerLimit = givenMin;
        upperLimit = givenMax;
        return true;
    }
    return false;
}

double ThresholdLimiter::transform(double givenSample) const {
    if (givenSample < lowerLimit) {return lowerLimit;}
    if (givenSample > upperLimit) {return upperLimit;}
    return givenSample;
}

ProcessingModule* ThresholdLimiter::clone() const {return new ThresholdLimiter(*this);}

std::string ThresholdLimiter::serialize() const {
    return "LIMITER;min=" + std::to_string(lowerLimit) + ";max=" + std::to_string(upperLimit);
}
