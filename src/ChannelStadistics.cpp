#include "ChannelStadistics.h"
#include <cmath>

ChannelStadistics::ChannelStadistics()
    : average(0.0),
      variation(0.0),
      standardDesviation(0.0),
      minimum(0.0),
      maximum(0.0),
      totalEnergy(0.0) {}

ChannelStadistics::ChannelStadistics(const double* initOfMemory, int quantityOfElements)
    : average(0.0),
      variation(0.0),
      standardDesviation(0.0),
      minimum(0.0),
      maximum(0.0),
      totalEnergy(0.0) {
    updateStadistics(initOfMemory, quantityOfElements);
}

bool ChannelStadistics::updateStadistics(const double* initOfMemory, int quantityOfElements) {
    if (initOfMemory == nullptr || quantityOfElements <= 0) {
        return false;
    }

    // Pass 1: mean, total energy, minimum, and maximum
    double sumAverage = 0.0;
    double sumEnergy = 0.0;
    double tempMin = *initOfMemory;
    double tempMax = *initOfMemory;

    for (int n = 0; n < quantityOfElements; ++n) {
        double value = *(initOfMemory + n);
        sumAverage += value;
        sumEnergy += value * value;
        if (value < tempMin) {
            tempMin = value;
        }
        if (value > tempMax) {
            tempMax = value;
        }
    }

    average = sumAverage / static_cast<double>(quantityOfElements);
    totalEnergy = sumEnergy;
    minimum = tempMin;
    maximum = tempMax;

    // Pass 2: Bessel-corrected sample variance and standard deviation
    if (quantityOfElements == 1) {
        variation = 0.0;
        standardDesviation = 0.0;
    } else {
        double sumVariance = 0.0;
        for (int n = 0; n < quantityOfElements; ++n) {
            double diff = *(initOfMemory + n) - average;
            sumVariance += diff * diff;
        }
        variation = sumVariance / static_cast<double>(quantityOfElements - 1);
        standardDesviation = std::sqrt(variation);
    }

    return true;
}

double ChannelStadistics::getAverage() const {
    return average;
}

double ChannelStadistics::getVariation() const {
    return variation;
}

double ChannelStadistics::getStandardDesviation() const {
    return standardDesviation;
}

double ChannelStadistics::getMinimum() const {
    return minimum;
}

double ChannelStadistics::getMaximum() const {
    return maximum;
}

double ChannelStadistics::getTotalEnergy() const {
    return totalEnergy;
}
