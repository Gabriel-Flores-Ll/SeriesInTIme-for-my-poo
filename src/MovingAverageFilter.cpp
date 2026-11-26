#include "MovingAverageFilter.h"

MovingAverageFilter::MovingAverageFilter(int givenWindow) : WindowFilter("MovingAverageFilter", givenWindow) {}

double MovingAverageFilter::calculate() const {
    const int n = getValidSamples();
    const double* end = getBuffer() + n;     // only the real samples (warm-up)
    double sum = 0.0;
    for (const double* p = getBuffer(); p < end; ++p) {sum += *p;}
    return sum / n;
}

ProcessingModule* MovingAverageFilter::clone() const {return new MovingAverageFilter(*this);}

std::string MovingAverageFilter::serialize() const {
    return "MOVING_AVERAGE;W=" + std::to_string(getWindowSize());
}
