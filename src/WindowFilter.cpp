#include "WindowFilter.h"

int WindowFilter::correctWindow(int givenWindow) {
    if (givenWindow < 3) {return 3;}
    if (givenWindow % 2 == 0) {return givenWindow + 1;}
    return givenWindow;
}

WindowFilter::WindowFilter(const std::string& givenName, int givenWindow)
    : ProcessingModule(givenName),
      windowSize(correctWindow(givenWindow)),
      buffer(new double[windowSize]()),   // the () fills everything with 0.0
      writeIndex(0),
      validSamples(0) {}

// DEEP copy: it reserves its own buffer and copies the content (no shared addresses)
WindowFilter::WindowFilter(const WindowFilter& other)
    : ProcessingModule(other),
      windowSize(other.windowSize),
      buffer(new double[other.windowSize]),
      writeIndex(other.writeIndex),
      validSamples(other.validSamples) {
    for (int i = 0; i < windowSize; i++) {
        *(buffer + i) = *(other.buffer + i);
    }
}

WindowFilter& WindowFilter::operator=(const WindowFilter& other) {
    if (this != &other) {                                  // protection against self-assignment
        double* newBuffer = new double[other.windowSize];
        for (int i = 0; i < other.windowSize; i++) {
            *(newBuffer + i) = *(other.buffer + i);
        }
        delete[] buffer;                                   // only now the old buffer is freed
        buffer = newBuffer;
        windowSize = other.windowSize;
        writeIndex = other.writeIndex;
        validSamples = other.validSamples;
        ProcessingModule::operator=(other);                // copies the name and the active flag
    }
    return *this;
}

WindowFilter::~WindowFilter() {delete[] buffer;}

int WindowFilter::getWindowSize() const {return windowSize;}
int WindowFilter::getValidSamples() const {return validSamples;}
const double* WindowFilter::getBuffer() const {return buffer;}

double WindowFilter::recentSample(int k) const {
    if (k < 0 || k >= validSamples) {return 0.0;}
    int position = writeIndex - 1 - k;
    if (position < 0) {position += windowSize;}            // wraps around the buffer
    return *(buffer + position);
}

double WindowFilter::processSample(double givenSample) {
    if (!isActive()) {return givenSample;}                 // bypass

    *(buffer + writeIndex) = givenSample;                  // overwrites the oldest one when the buffer is full
    writeIndex = (writeIndex + 1) % windowSize;
    if (validSamples < windowSize) {validSamples++;}

    return calculate();                                    // dynamic dispatch to the leaf class
}

void WindowFilter::reset() {
    for (int i = 0; i < windowSize; i++) {*(buffer + i) = 0.0;}
    writeIndex = 0;
    validSamples = 0;
}
