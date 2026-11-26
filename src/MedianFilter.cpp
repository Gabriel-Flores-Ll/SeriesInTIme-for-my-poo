#include "MedianFilter.h"

MedianFilter::MedianFilter(int givenWindow)
    : WindowFilter("MedianFilter", givenWindow) {}

void MedianFilter::insertionSort(double* givenArray, int givenQuantity) {
    for (int i = 1; i < givenQuantity; ++i) {
        const double key = *(givenArray + i);
        int j = i - 1;
        while (j >= 0 && *(givenArray + j) > key) {
            *(givenArray + j + 1) = *(givenArray + j);
            --j;
        }
        *(givenArray + j + 1) = key;
    }
}

double MedianFilter::calculate() const {
    const int n = getValidSamples();
    if (n <= 0) {
        return 0.0;
    }

    double stackCopy[kStackFastPathLimit];
    double* heapCopy = nullptr;
    double* work = stackCopy;

    if (n > kStackFastPathLimit) {
        heapCopy = new double[n];
        work = heapCopy;
    }

    const double* original = getBuffer();
    for (int i = 0; i < n; ++i) {
        *(work + i) = *(original + i);
    }

    insertionSort(work, n);

    double median = 0.0;
    if (n % 2 == 1) {
        median = *(work + (n / 2));
    } else {
        median = 0.5 * (*(work + (n / 2) - 1) + *(work + (n / 2)));
    }

    delete[] heapCopy;
    return median;
}

ProcessingModule* MedianFilter::clone() const {
    return new MedianFilter(*this);
}

std::string MedianFilter::serialize() const {
    return "MEDIAN;W=" + std::to_string(getWindowSize()) +
           ";active=" + (isActive() ? "1" : "0");
}
