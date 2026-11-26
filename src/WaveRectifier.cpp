#include "WaveRectifier.h"

#include <cmath>

WaveRectifier::WaveRectifier(RectifierMode givenMode)
    : PointTransformer("WaveRectifier", 1.0), mode(givenMode) {}

RectifierMode WaveRectifier::getMode() const {return mode;}
void WaveRectifier::setMode(RectifierMode givenMode) {mode = givenMode;}

double WaveRectifier::transform(double givenSample) const {
    if (mode == RectifierMode::FullWave) {return std::fabs(givenSample);}
    return givenSample > 0.0 ? givenSample : 0.0;
}

ProcessingModule* WaveRectifier::clone() const {return new WaveRectifier(*this);}

std::string WaveRectifier::serialize() const {
    return std::string("RECTIFIER;mode=") + (mode == RectifierMode::FullWave ? "FULL_WAVE" : "HALF_WAVE");
}
