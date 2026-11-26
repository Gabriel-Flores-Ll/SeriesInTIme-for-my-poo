#include "EventDetector.h"

#include <cmath>
#include <stdexcept>

EventDetector::EventDetector(const std::string& givenName,
                             const std::string& givenBufferSelector)
    : name(givenName),
      bufferSelector(givenBufferSelector) {
    if (name.empty()) {
        throw std::invalid_argument("EventDetector: name cannot be empty.");
    }
    if (bufferSelector != "processed" && bufferSelector != "raw") {
        throw std::invalid_argument("EventDetector: bufferSelector must be \"processed\" or \"raw\".");
    }
}

double EventDetector::sampleAt(const ChannelSignal& signal, int index) const {
    if (bufferSelector == "processed") {
        return signal.getProcessedElement(index);
    }
    return signal.getRawElement(index);
}

void EventDetector::computeBaseline(const ChannelSignal& signal,
                                    double& outMean,
                                    double& outStdDev,
                                    double& outTotalEnergy) const {
    if (bufferSelector == "processed") {
        const ChannelStadistics& stats = signal.getProcessedStatistics();
        outMean = stats.getAverage();
        outStdDev = stats.getStandardDesviation();
        outTotalEnergy = stats.getTotalEnergy();
        return;
    }

    const int n = signal.getNumberOfSamples();
    if (n <= 0) {
        outMean = 0.0;
        outStdDev = 0.0;
        outTotalEnergy = 0.0;
        return;
    }

    double sum = 0.0;
    double energy = 0.0;
    for (int i = 0; i < n; ++i) {
        const double val = signal.getRawElement(i);
        sum += val;
        energy += val * val;
    }
    outMean = sum / static_cast<double>(n);
    outTotalEnergy = energy;

    if (n == 1) {
        outStdDev = 0.0;
        return;
    }

    double sumSqDiff = 0.0;
    for (int i = 0; i < n; ++i) {
        const double diff = signal.getRawElement(i) - outMean;
        sumSqDiff += diff * diff;
    }
    const double variance = sumSqDiff / static_cast<double>(n - 1);
    outStdDev = (variance > 0.0) ? std::sqrt(variance) : 0.0;
}

const std::string& EventDetector::getName() const {
    return name;
}

const std::string& EventDetector::getBufferSelector() const {
    return bufferSelector;
}

std::ostream& operator<<(std::ostream& os, const EventDetector& detector) {
    os << detector.getDescription();
    return os;
}