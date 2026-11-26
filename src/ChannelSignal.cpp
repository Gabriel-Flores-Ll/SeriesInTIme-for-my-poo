#include "ChannelSignal.h"

#include "ProcessingModule.h"
#include "ProcessingPipeline.h"

#include <limits>

ChannelSignal::ChannelSignal(const std::string& givenCode,
                             const std::string& givenUnit,
                             double givenSR,
                             const double* inicialData,
                             int providedNumOfElements)
    : nativeMetaData(givenCode, givenUnit, givenSR),
      nativeStadistics(),
      rawSamples(),
      processedSamples(),
      totalOfSamples(0),
      statsDirty(true) {
    if (inicialData != nullptr && providedNumOfElements >= 2) {
        rawSamples.setElementsInRow(inicialData, providedNumOfElements);
        processedSamples = rawSamples;
        totalOfSamples = providedNumOfElements;
    } else {
        const double fallback[2] = {0.0, 0.0};
        rawSamples.setElementsInRow(fallback, 2);
        processedSamples = rawSamples;
        totalOfSamples = 2;
    }

    nativeStadistics.updateStadistics(processedSamples.getData(), totalOfSamples);
    statsDirty = false;
}

const DinamicArr* ChannelSignal::selectBuffer(const std::string& selectionOfArr) const {
    if (selectionOfArr == "raw") {
        return &rawSamples;
    }
    if (selectionOfArr == "processed") {
        return &processedSamples;
    }
    return nullptr;
}

bool ChannelSignal::isValidSubRange(int initOfRange, int numberOfElements) const {
    return initOfRange >= 0 &&
           numberOfElements > 0 &&
           initOfRange <= totalOfSamples - numberOfElements;
}

const ChannelStadistics& ChannelSignal::getProcessedStatistics() const {
    if (statsDirty) {
        nativeStadistics.updateStadistics(processedSamples.getData(), totalOfSamples);
        statsDirty = false;
    }
    return nativeStadistics;
}

const MetadataSensor& ChannelSignal::getMetadata() const {
    return nativeMetaData;
}

double ChannelSignal::getRawElement(int index) const {
    return rawSamples.getElement(index);
}

double ChannelSignal::getProcessedElement(int index) const {
    return processedSamples.getElement(index);
}

int ChannelSignal::getNumberOfSamples() const {
    return totalOfSamples;
}

double ChannelSignal::getSamplingRate() const {
    return nativeMetaData.getSamplingRate();
}

bool ChannelSignal::setProcessedElement(double providedElement, int indexProvided) {
    if (processedSamples.setElement(providedElement, indexProvided)) {
        statsDirty = true;
        return true;
    }
    return false;
}

void ChannelSignal::restoreProcessedSamples() {
    processedSamples = rawSamples;
    statsDirty = true;
}

bool ChannelSignal::applyModule(ProcessingModule& module, const std::string& sourceBuffer) {
    const DinamicArr* source = selectBuffer(sourceBuffer);
    if (source == nullptr) {
        return false;
    }

    module.reset();

    const double* srcData = source->getData();
    double* filtered = new double[totalOfSamples];
    try {
        const double* srcEnd = srcData + totalOfSamples;
        double* dst = filtered;
        for (const double* src = srcData; src < srcEnd; ++src, ++dst) {
            *dst = module.processSample(*src);
        }
        processedSamples.setElementsInRow(filtered, totalOfSamples);
    } catch (...) {
        delete[] filtered;
        throw;
    }
    delete[] filtered;

    statsDirty = true;
    return true;
}

bool ChannelSignal::applyPipeline(ProcessingPipeline& pipeline, const std::string& sourceBuffer) {
    const DinamicArr* source = selectBuffer(sourceBuffer);
    if (source == nullptr) {
        return false;
    }

    pipeline.reset();

    if (pipeline.isEmpty()) {
        if (sourceBuffer == "raw") {
            restoreProcessedSamples();
        }
        return true;
    }

    const double* srcData = source->getData();
    double* filtered = new double[totalOfSamples];
    try {
        const double* srcEnd = srcData + totalOfSamples;
        double* dst = filtered;
        for (const double* src = srcData; src < srcEnd; ++src, ++dst) {
            *dst = pipeline.processSample(*src);
        }
        processedSamples.setElementsInRow(filtered, totalOfSamples);
    } catch (...) {
        delete[] filtered;
        throw;
    }
    delete[] filtered;

    statsDirty = true;
    return true;
}

double ChannelSignal::subRangeAverage(int initOfRange,
                                      int numberOfElements,
                                      const std::string& selectionOfArr) const {
    const DinamicArr* buffer = selectBuffer(selectionOfArr);
    if (buffer == nullptr || !isValidSubRange(initOfRange, numberOfElements)) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const double* data = buffer->getData();
    double sumAverage = 0.0;
    for (int n = 0; n < numberOfElements; ++n) {
        sumAverage += *(data + initOfRange + n);
    }
    return sumAverage / static_cast<double>(numberOfElements);
}

double ChannelSignal::subRangeMinimum(int initOfRange,
                                      int numberOfElements,
                                      const std::string& selectionOfArr) const {
    const DinamicArr* buffer = selectBuffer(selectionOfArr);
    if (buffer == nullptr || !isValidSubRange(initOfRange, numberOfElements)) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const double* data = buffer->getData();
    double tempMin = *(data + initOfRange);
    for (int n = 1; n < numberOfElements; ++n) {
        double value = *(data + initOfRange + n);
        if (value < tempMin) {
            tempMin = value;
        }
    }
    return tempMin;
}

double ChannelSignal::subRangeMaximum(int initOfRange,
                                      int numberOfElements,
                                      const std::string& selectionOfArr) const {
    const DinamicArr* buffer = selectBuffer(selectionOfArr);
    if (buffer == nullptr || !isValidSubRange(initOfRange, numberOfElements)) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const double* data = buffer->getData();
    double tempMax = *(data + initOfRange);
    for (int n = 1; n < numberOfElements; ++n) {
        double value = *(data + initOfRange + n);
        if (value > tempMax) {
            tempMax = value;
        }
    }
    return tempMax;
}

double ChannelSignal::subRangeEnergy(int initOfRange,
                                     int numberOfElements,
                                     const std::string& selectionOfArr) const {
    const DinamicArr* buffer = selectBuffer(selectionOfArr);
    if (buffer == nullptr || !isValidSubRange(initOfRange, numberOfElements)) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const double* data = buffer->getData();
    double sumEnergy = 0.0;
    for (int n = 0; n < numberOfElements; ++n) {
        double value = *(data + initOfRange + n);
        sumEnergy += value * value;
    }
    return sumEnergy;
}