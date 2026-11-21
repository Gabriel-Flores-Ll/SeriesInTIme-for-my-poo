#include "DinamicArr.h"
#include <limits>

DinamicArr::DinamicArr()
    : numberOfElements(0),
      nativeArray(nullptr) {}

DinamicArr::DinamicArr(int providedNumOfElements)
    : numberOfElements(0),
      nativeArray(nullptr) {
    if (providedNumOfElements > 0) {
        nativeArray = new double[providedNumOfElements]{};
        numberOfElements = providedNumOfElements;
    }
}

DinamicArr::DinamicArr(const double* rowOfElements, int providedNumberOfElements)
    : numberOfElements(0),
      nativeArray(nullptr) {
    setElementsInRow(rowOfElements, providedNumberOfElements);
}

DinamicArr::DinamicArr(const DinamicArr& other)
    : numberOfElements(0),
      nativeArray(nullptr) {
    if (other.nativeArray != nullptr && other.numberOfElements > 0) {
        setElementsInRow(other.nativeArray, other.numberOfElements);
    }
}

DinamicArr::~DinamicArr() {
    delete[] nativeArray;
    nativeArray = nullptr;
    numberOfElements = 0;
}

DinamicArr& DinamicArr::operator=(const DinamicArr& other) {
    if (this != &other) {
        if (other.nativeArray != nullptr && other.numberOfElements > 0) {
            double* newBlock = new double[other.numberOfElements];
            for (int i = 0; i < other.numberOfElements; ++i) {
                *(newBlock + i) = *(other.nativeArray + i);
            }
            delete[] nativeArray;
            nativeArray = newBlock;
            numberOfElements = other.numberOfElements;
        } else {
            delete[] nativeArray;
            nativeArray = nullptr;
            numberOfElements = 0;
        }
    }
    return *this;
}

bool DinamicArr::setElement(double elementProvided, int indexProvided) {
    if (nativeArray != nullptr && indexProvided >= 0 && indexProvided < numberOfElements) {
        *(nativeArray + indexProvided) = elementProvided;
        return true;
    }
    return false;
}

bool DinamicArr::setElementsInRow(const double* rowOfElements, int providedNumberOfElements) {
    if (rowOfElements == nullptr || providedNumberOfElements <= 0) {
        return false;
    }

    double* newBlock = new double[providedNumberOfElements];
    for (int i = 0; i < providedNumberOfElements; ++i) {
        *(newBlock + i) = *(rowOfElements + i);
    }

    delete[] nativeArray;
    nativeArray = newBlock;
    numberOfElements = providedNumberOfElements;
    return true;
}

int DinamicArr::getNumberOfElements() const {
    return numberOfElements;
}

double DinamicArr::getElement(int index) const {
    if (nativeArray != nullptr && index >= 0 && index < numberOfElements) {
        return *(nativeArray + index);
    }
    return std::numeric_limits<double>::quiet_NaN();
}

const double* DinamicArr::getData() const {
    return nativeArray;
}
