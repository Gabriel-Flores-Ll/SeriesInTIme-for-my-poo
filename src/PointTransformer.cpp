#include "PointTransformer.h"

PointTransformer::PointTransformer(const std::string& givenName, double givenFactor)
    : ProcessingModule(givenName), scaleFactor(1.0) {
    setScaleFactor(givenFactor);
}

double PointTransformer::getScaleFactor() const {return scaleFactor;}

bool PointTransformer::setScaleFactor(double givenFactor) {
    if (givenFactor == 0.0) {
        scaleFactor = 1.0;
        return false;
    }
    scaleFactor = givenFactor;
    return true;
}

double PointTransformer::processSample(double givenSample) {
    if (!isActive()) {return givenSample;}
    return transform(givenSample) * scaleFactor;
}

void PointTransformer::reset() {}
