#include "ChannelSignal.h"
#include "MetadataSensor.h"

ChannelSignal::ChannelSignal(std::string givenCode, std::string givenUnit, double givenSR, double* inicialData, int ProvidedNumOfElements) : nativeMetaData(givenCode, givenUnit, givenSR), nativeStadistics(inicialData, ProvidedNumOfElements){
    rawSamples.setElementsInRow(inicialData, ProvidedNumOfElements);
    processedSamples = rawSamples;
    totalOfSamples = ProvidedNumOfElements;
    dirtyFlag = true;
}

ChannelSignal::ChannelSignal(const ChannelSignal &other): nativeMetaData(other.nativeMetaData){*this = other;}

ChannelSignal ChannelSignal::operator=(const ChannelSignal &other) {
    nativeMetaData = other.nativeMetaData;
    nativeStadistics = other.nativeStadistics;
    rawSamples = other.rawSamples;
    processedSamples = other.processedSamples;
    totalOfSamples = other.totalOfSamples;
    dirtyFlag = other.dirtyFlag;
    return *this;
}

void ChannelSignal::updateInternalStadistics(std::string selectionOfArr) {
    if(selectionOfArr == "raw") {
        double* tempSamples = rawSamples.getAllElements();
        this->nativeStadistics.updateStadistics(tempSamples, totalOfSamples);
        delete [] tempSamples;
    }
    else if(selectionOfArr == "processed") {
        double* tempSamples = processedSamples.getAllElements();
        this->nativeStadistics.updateStadistics(tempSamples, totalOfSamples);
        delete [] tempSamples;
    }
    dirtyFlag = true;
}

double ChannelSignal::getRawElement (int index) {return rawSamples.getElement(index);}
double ChannelSignal::getProcesedElement(int index){return processedSamples.getElement(index);}

bool ChannelSignal::setRawElement(double providedElement, int indexProvided) {return rawSamples.setElement(providedElement, indexProvided);dirtyFlag = false;};
bool ChannelSignal::setPracessedElement(double providedElement, int indexProvided){return processedSamples.setElement(providedElement, indexProvided);dirtyFlag = false;}

bool ChannelSignal::restoreProcessedSamples(){processedSamples = rawSamples; return true;dirtyFlag = false;}

double ChannelSignal::subRangeAverage(int initOfRange, int numberOfElements, std::string selectionOfArr) {
    double sumAverage = 0;
    if(selectionOfArr == "raw"){
        for (int n = 0; n < numberOfElements; n++) {
            sumAverage += (rawSamples.getElement(initOfRange + n));
        }
        return sumAverage/numberOfElements;
    }
    else if(selectionOfArr == "processed"){
        for (int n = 0; n < numberOfElements; n++) {
            sumAverage += (processedSamples.getElement(initOfRange + n));
        }
        return sumAverage/numberOfElements;
    }
    return -1;
}  
double ChannelSignal::subRangeminimum(int initOfRange, int numberOfElements, std::string selectionOfArr) {
    if (selectionOfArr == "raw") {
        double tempmax = rawSamples.getElement(initOfRange);
        for(int n = 0; n < numberOfElements; n++) {if (rawSamples.getElement(initOfRange + n) > tempmax) {tempmax = rawSamples.getElement(initOfRange + n);}}
        return tempmax;
    }
    else if (selectionOfArr == "processed") {
        double tempmax = processedSamples.getElement(initOfRange);
        for(int n = 0; n < numberOfElements; n++) {if (processedSamples.getElement(initOfRange + n) > tempmax) {tempmax = processedSamples.getElement(initOfRange + n);}}
        return tempmax;
    }
    return -1;
}
double ChannelSignal::subRangemaximum(int initOfRange, int numberOfElements, std::string selectionOfArr) {
    if (selectionOfArr == "raw") {
        double tempmin = rawSamples.getElement(initOfRange);
        for(int n = 0; n < numberOfElements; n++) {if (rawSamples.getElement(initOfRange + n) < tempmin) {tempmin = rawSamples.getElement(initOfRange + n);}}
        return tempmin;
    }
    else if (selectionOfArr == "processed") {
        double tempmin = processedSamples.getElement(initOfRange);
        for(int n = 0; n < numberOfElements; n++) {if (processedSamples.getElement(initOfRange + n) < tempmin) {tempmin = processedSamples.getElement(initOfRange + n);}}
        return tempmin;
    }
    return -1;
}
double ChannelSignal::subRangeEnergy(int initOfRange, int numberOfElements, std::string selectionOfArr) {
    double sumEnergy = 0;
    if(selectionOfArr == "raw"){
        for (int n = 0; n < numberOfElements; n++) {
            sumEnergy += (rawSamples.getElement(initOfRange + n)) * (rawSamples.getElement(initOfRange + n));
        }
        return sumEnergy;
    }
    else if(selectionOfArr == "processed"){
        for (int n = 0; n < numberOfElements; n++) {
            sumEnergy += (processedSamples.getElement(initOfRange + n)) * (processedSamples.getElement(initOfRange + n));
        }
        return sumEnergy;
    }
    return -1;
}