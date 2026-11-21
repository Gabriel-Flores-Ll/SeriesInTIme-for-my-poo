#include "MetadataSensor.h"

MetadataSensor::MetadataSensor(const std::string& givenCode,
                               const std::string& givenUnit,
                               double givenSR)
    : codeOfChannel("UNKNOWN"),
      unitOfMeasure("uV"),
      samplingRate(1.0) {
    if (isAContinousSTR(givenCode, 1)) {
        codeOfChannel = givenCode;
    }
    if (isAContinousSTR(givenUnit, 1)) {
        unitOfMeasure = givenUnit;
    }
    if (givenSR > 0.0) {
        samplingRate = givenSR;
    }
}

bool MetadataSensor::isAContinousSTR(const std::string& givenSTR, int requiredContinuity) const {
    if (givenSTR.empty() || requiredContinuity <= 0) {
        return false;
    }

    int continuity = 0;
    for (char c : givenSTR) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            return false;
        }
        continuity += 1;
    }

    return continuity >= requiredContinuity;
}

bool MetadataSensor::setCodeOfChannel(const std::string& givenCode) {
    if (isAContinousSTR(givenCode, 1)) {
        codeOfChannel = givenCode;
        return true;
    }
    return false;
}

bool MetadataSensor::setUnitOfMeasure(const std::string& givenUnit) {
    if (isAContinousSTR(givenUnit, 1)) {
        unitOfMeasure = givenUnit;
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

const std::string& MetadataSensor::getCodeOfChannel() const {
    return codeOfChannel;
}

const std::string& MetadataSensor::getUnitOfMeasure() const {
    return unitOfMeasure;
}

double MetadataSensor::getSamplingRate() const {
    return samplingRate;
}

int MetadataSensor::ConvertIndexSecs(double givenSecond) const {
    if (givenSecond < 0.0) {
        return 0;
    }
    return static_cast<int>(samplingRate * givenSecond);
}

double MetadataSensor::ConvertIndexSecs(int givenIndex) const {
    if (givenIndex < 0) {
        return 0.0;
    }
    return static_cast<double>(givenIndex) / samplingRate;
}
