#include "MetadataSensor.h"

MetadataSensor::MetadataSensor(std::string givenCode, std::string givenUnit, double givenSR) {
    if(isAContinousSTR(givenCode, 1)) {codeOfChannel = givenCode;}
    if(isAContinousSTR(givenUnit, 1)) {unitOfMeasure = givenUnit;}
    if(givenSR > 0.0) {samplingRate = givenSR;}
};

bool MetadataSensor::isAContinousSTR(std::string &givenSTR, int requiredContinuity) const {
    if (!givenSTR.empty()) {
        int continuity = 0;
        for( char& c : givenSTR) {
            if (c != ' ') {
                continuity += 1;
            } else {continuity = 0;}
        } 
        
        if(continuity >= requiredContinuity) {return true;} 
    }
    return false;
};

bool MetadataSensor::setCodeOfChannel(std::string givenCode) {
    if (isAContinousSTR(givenCode, 1)) {
        codeOfChannel = givenCode;
        return true;
    }
    return false;
}

bool MetadataSensor::setSamplingRate(double givenSR) {
    if (givenSR > 0.0) {
        samplingRate = givenSR;
        return true;
    }
    return false;
}

bool MetadataSensor::setUnitOfMeasure(std::string givenUnit) {
    if (isAContinousSTR(givenUnit, 1)) {
        unitOfMeasure = givenUnit;
        return true;
    }
    return false;
}

std::string MetadataSensor::getCodeOfChanne(){return codeOfChannel;};
std::string MetadataSensor::getUnitOfMeasure() {return unitOfMeasure;};
double MetadataSensor::getSamplingRate() {return samplingRate;};

int MetadataSensor::ConvertIndexSecs(double givenSecond) const {return samplingRate * givenSecond;}

double MetadataSensor::ConvertIndexSecs(int givenIndex) const {return givenIndex / samplingRate;}
