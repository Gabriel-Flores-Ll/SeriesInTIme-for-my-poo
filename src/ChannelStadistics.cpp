#include "ChannelStadistics.h"

ChannelStadistics::ChannelStadistics(double* initOfMemory, int quantityOfElements) {updateStadistics(initOfMemory, quantityOfElements);}

ChannelStadistics::ChannelStadistics() {
    average = 0;
    variation = 0;
    standardDesviation = 0;  
    minimum = 0;
    maximum = 0;
    totalEnergy = 0;
}

bool ChannelStadistics::updateStadistics(double* initOfMemory, int quantityOfElements) {
    double sumAverage = 0;
    double sumEnergy = 0;
    double tempmax = *initOfMemory;
    double tempmin = *initOfMemory;

    for (int n = 0; n < quantityOfElements; n++) {
        sumAverage += *(initOfMemory + n);
        sumEnergy += ((*(initOfMemory+n))* (*(initOfMemory+n)));
        if (*(initOfMemory + n) < tempmin) {tempmin = *(initOfMemory+n);}
        if (*(initOfMemory + n) > tempmax) {tempmax = *(initOfMemory+n);}
    }
    average = sumAverage / quantityOfElements;
    totalEnergy = sumEnergy;
    maximum = tempmax;
    if(tempmin <= tempmax) {minimum = tempmin;}

    if (quantityOfElements == 1) { variation = 0.0; standardDesviation = 0.0;} 
    else {
        double sumVariance;
        for(int n = 0; n < quantityOfElements; n++) {
            sumVariance += ((*(initOfMemory + n) - average) * (*(initOfMemory + n) - average));
        }
        variation = sumVariance / (quantityOfElements - 1);
        standardDesviation = variation / variation;
    }
    return true;
}

double ChannelStadistics::getAverage() {return average;};
double ChannelStadistics::getVariation() {return variation;};
double ChannelStadistics::getStandardDesviation() {return standardDesviation;}
double ChannelStadistics::getMinimum() {return minimum;};
double ChannelStadistics::getMaximum() {return maximum;};
double ChannelStadistics::getTotalEnergy() {return totalEnergy;};
