#include "WindowFilter.h"

#include <cmath>

int WindowFilter::correctWindow(int givenWindow) {
    if (givenWindow < kMinWindowSize) {
        return kMinWindowSize;
    }
    if (givenWindow >= kMaxWindowSize) {
        return kMaxWindowSize;
    }
    if (givenWindow % 2 == 0) {
        return givenWindow + 1;
    }
    return givenWindow;
}

WindowFilter::WindowFilter(const std::string& givenName, int givenWindow)
    : ProcessingModule(givenName),
      windowSize(correctWindow(givenWindow)),
      buffer(new double[windowSize]()),
      writeIndex(0),
      validSamples(0) {}

WindowFilter::WindowFilter(const WindowFilter& other)
    : ProcessingModule(other),
      windowSize(other.windowSize),
      buffer(new double[other.windowSize]),
      writeIndex(other.writeIndex),
      validSamples(other.validSamples) {
    for (int i = 0; i < windowSize; ++i) {
        *(buffer + i) = *(other.buffer + i);
    }
}

WindowFilter& WindowFilter::operator=(const WindowFilter& other) {
    if (this != &other) {
        double* newBuffer = new double[other.windowSize];
        for (int i = 0; i < other.windowSize; ++i) {
            *(newBuffer + i) = *(other.buffer + i);
        }
        try {
            ProcessingModule::operator=(other);
        } catch (...) {
            delete[] newBuffer;
            throw;
        }
        delete[] buffer;
        buffer = newBuffer;
        windowSize = other.windowSize;
        writeIndex = other.writeIndex;
        validSamples = other.validSamples;
    }
    return *this;
}

WindowFilter::~WindowFilter() {
    delete[] buffer;
}

int WindowFilter::getWindowSize() const {
    return windowSize;
}

int WindowFilter::getValidSamples() const {
    return validSamples;
}

const double* WindowFilter::getBuffer() const {
    return buffer;
}

double WindowFilter::recentSample(int k) const {
    if (k < 0 || k >= validSamples) {
        return 0.0;
    }
    int position = writeIndex - 1 - k;
    if (position < 0) {
        position += windowSize;
    }
    return *(buffer + position);
}

double WindowFilter::processSample(double givenSample) {
    if (!isActive() || !std::isfinite(givenSample)) {
        return givenSample;
    }

    *(buffer + writeIndex) = givenSample;
    writeIndex = (writeIndex + 1) % windowSize;
    if (validSamples < windowSize) {
        ++validSamples;
    }

    return calculate();
}

void WindowFilter::reset() {
    for (int i = 0; i < windowSize; ++i) {
        *(buffer + i) = 0.0;
    }
    writeIndex = 0;
    validSamples = 0;
}
