#include "MovingAverageFilter.h"

MovingAverageFilter::MovingAverageFilter(int givenWindow)
    : WindowFilter("MovingAverageFilter", givenWindow) {}

double MovingAverageFilter::calculate() const {
    const int n = getValidSamples();
    if (n <= 0) {
        return 0.0;
    }
    const double* end = getBuffer() + n;
    double sum = 0.0;
    for (const double* p = getBuffer(); p < end; ++p) {
        sum += *p;
    }
    return sum / static_cast<double>(n);
}

ProcessingModule* MovingAverageFilter::clone() const {
    return new MovingAverageFilter(*this);
}

std::string MovingAverageFilter::serialize() const {
    return "MOVING_AVERAGE;W=" + std::to_string(getWindowSize()) +
           ";active=" + (isActive() ? "1" : "0");
}
