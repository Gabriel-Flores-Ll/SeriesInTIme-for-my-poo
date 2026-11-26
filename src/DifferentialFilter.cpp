#include "DifferentialFilter.h"

DifferentialFilter::DifferentialFilter(int givenWindow) : WindowFilter("DifferentialFilter", givenWindow) {}

double DifferentialFilter::calculate() const {
    const int n = getValidSamples();
    if (n == 1) {return 0.0;}                              // there is no previous sample to compare with

    const double current = recentSample(0);                // x[n]
    const double previous = recentSample(1);               // x[n-1]
    if (n == 2) {return current - previous;}               // finite difference

    const double oldest = recentSample(2);                 // x[n-2]
    return previous * previous - oldest * current;         // TKEO
}

ProcessingModule* DifferentialFilter::clone() const {return new DifferentialFilter(*this);}

std::string DifferentialFilter::serialize() const {
    return "DIFFERENTIAL_TKEO;W=" + std::to_string(getWindowSize());
}
