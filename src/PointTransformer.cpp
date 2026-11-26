#include "PointTransformer.h"

#include <cmath>

PointTransformer::PointTransformer(const std::string& givenName, double givenFactor)
    : ProcessingModule(givenName),
      scaleFactor((std::isfinite(givenFactor) && givenFactor != 0.0) ? givenFactor : 1.0) {}

double PointTransformer::getScaleFactor() const {
    return scaleFactor;
}

bool PointTransformer::setScaleFactor(double givenFactor) {
    if (!std::isfinite(givenFactor) || givenFactor == 0.0) {
        return false;
    }
    scaleFactor = givenFactor;
    return true;
}

double PointTransformer::processSample(double givenSample) {
    if (!isActive() || !std::isfinite(givenSample)) {
        return givenSample;
    }
    return transform(givenSample) * scaleFactor;
}

void PointTransformer::reset() {}
