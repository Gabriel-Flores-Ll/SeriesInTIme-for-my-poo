#include "WaveRectifier.h"

#include <cmath>
#include <iomanip>
#include <sstream>

WaveRectifier::WaveRectifier(RectifierMode givenMode, double givenScaleFactor)
    : PointTransformer("WaveRectifier",
                       (std::isfinite(givenScaleFactor) && givenScaleFactor > 0.0)
                           ? givenScaleFactor
                           : 1.0),
      mode(givenMode) {}

RectifierMode WaveRectifier::getMode() const {
    return mode;
}

void WaveRectifier::setMode(RectifierMode givenMode) {
    mode = givenMode;
}

bool WaveRectifier::setScaleFactor(double givenFactor) {
    if (!std::isfinite(givenFactor) || givenFactor <= 0.0) {
        return false;
    }
    return PointTransformer::setScaleFactor(givenFactor);
}

double WaveRectifier::transform(double givenSample) const {
    if (mode == RectifierMode::FullWave) {
        return std::fabs(givenSample);
    }
    return (givenSample > 0.0) ? givenSample : 0.0;
}

ProcessingModule* WaveRectifier::clone() const {
    return new WaveRectifier(*this);
}

std::string WaveRectifier::serialize() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6)
        << "RECTIFIER;mode="
        << (mode == RectifierMode::FullWave ? "FULL_WAVE" : "HALF_WAVE")
        << ";scale=" << getScaleFactor()
        << ";active=" << (isActive() ? "1" : "0");
    return oss.str();
}
