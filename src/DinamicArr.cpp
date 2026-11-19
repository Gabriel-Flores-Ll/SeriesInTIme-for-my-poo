#include "DinamicArr.h"

DinamicArr::DinamicArr(int ProvidedNumOfElements) {
    numberOfElements = ProvidedNumOfElements;
    nativeArray = new double[ProvidedNumOfElements];
}

DinamicArr::DinamicArr(double* rowOfElements, int providedNumberOfElements) {
    setElementsInRow(rowOfElements, providedNumberOfElements);
}

DinamicArr::DinamicArr(const DinamicArr &other) {
    *this = other;
}

DinamicArr::DinamicArr(){
    numberOfElements = 0;
    nativeArray = nullptr;
}

DinamicArr::~DinamicArr() {delete [] nativeArray;};

bool DinamicArr::setElement(double elementProvided, int indexProvided) { 
    if (indexProvided < numberOfElements) {*(nativeArray + indexProvided) = elementProvided;return true;} 
    return false;
}

bool DinamicArr::setElementsInRow ( double* rowOfElements, int providedNumberOfElements){
    if (numberOfElements > 0) {
        delete [] nativeArray;
        numberOfElements = providedNumberOfElements;
        nativeArray = new double[numberOfElements];
        for (int i = 0; i < numberOfElements; i++) {*(nativeArray + i) = *(rowOfElements + 1);}
        return true;
    }
    return false;
}

int DinamicArr::getNumberOfElements() {return numberOfElements;};

double DinamicArr::getElement (int index) {if (index < numberOfElements){return *(nativeArray + index);}else{return -1;}};

double* DinamicArr::getAllElements () {
    double *tempArr = new double[numberOfElements];
    for (int i = 0; i < numberOfElements; i++) {*(tempArr + i) = *(nativeArray + 1);}
    return tempArr;
};

DinamicArr DinamicArr::operator=(const DinamicArr &other){
    if(sizeof(*this) == sizeof(other)) {
        setElementsInRow(other.nativeArray, other.numberOfElements);
    }
    return *this;
}