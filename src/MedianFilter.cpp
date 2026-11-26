#include "MedianFilter.h"

MedianFilter::MedianFilter(int givenWindow) : WindowFilter("MedianFilter", givenWindow) {}

void MedianFilter::insertionSort(double* givenArray, int givenQuantity) {
    for (int i = 1; i < givenQuantity; i++) {
        double key = *(givenArray + i);
        int j = i - 1;
        while (j >= 0 && *(givenArray + j) > key) {        // shifts to the right the bigger ones
            *(givenArray + j + 1) = *(givenArray + j);
            j--;
        }
        *(givenArray + j + 1) = key;
    }
}

double MedianFilter::calculate() const {
    const int n = getValidSamples();

    double* copy = new double[n];                          // temporary auxiliary array
    const double* original = getBuffer();
    for (int i = 0; i < n; i++) {*(copy + i) = *(original + i);}

    insertionSort(copy, n);                                // the COPY is sorted, never the buffer
    double median = *(copy + (n - 1) / 2);                 // central element

    delete[] copy;
    return median;
}

ProcessingModule* MedianFilter::clone() const {return new MedianFilter(*this);}

std::string MedianFilter::serialize() const {
    return "MEDIAN;W=" + std::to_string(getWindowSize());
}
