#include "DifferentialFilter.h"

DifferentialFilter::DifferentialFilter()
    : WindowFilter("DifferentialFilter", 3) {}

double DifferentialFilter::calculate() const {
    if (getValidSamples() < 3) {
        return 0.0;
    }

    const double current = recentSample(0);   // x[n]
    const double previous = recentSample(1);  // x[n - 1]
    const double oldest = recentSample(2);    // x[n - 2]
    return (previous * previous) - (oldest * current);
}

ProcessingModule* DifferentialFilter::clone() const {
    return new DifferentialFilter(*this);
}

std::string DifferentialFilter::serialize() const {
    return "DIFFERENTIAL_TKEO;W=3;active=" + std::string(isActive() ? "1" : "0");
}
