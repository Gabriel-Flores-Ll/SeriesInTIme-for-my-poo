#include "SlicingWindow.h"
#include "ChannelSignal.h"
#include <limits>

SlicingWindow::SlicingWindow(const ChannelSignal* channelProvided,
                             int windowLenghtProvided,
                             int strideProvided)
    : channelAsocied(channelProvided),
      lengthOfTheChannel(channelProvided != nullptr ? channelProvided->getNumberOfSamples() : 0),
      initIndex(0),
      windowLenght(0),
      stride(1) {
    if (strideProvided >= 1) {
        stride = strideProvided;
    }

    if (channelAsocied != nullptr && lengthOfTheChannel > 0) {
        if (windowLenghtProvided < 1) {
            windowLenght = 1;
        } else if (windowLenghtProvided > lengthOfTheChannel) {
            windowLenght = lengthOfTheChannel;
        } else {
            windowLenght = windowLenghtProvided;
        }
    }
}

void SlicingWindow::resetInitIndex() {
    initIndex = 0;
}

bool SlicingWindow::setInitIndex(int indexProvided) {
    if (channelAsocied != nullptr &&
        indexProvided >= 0 &&
        indexProvided <= lengthOfTheChannel - windowLenght) {
        initIndex = indexProvided;
        return true;
    }
    return false;
}

bool SlicingWindow::canStrideForward() const {
    return channelAsocied != nullptr &&
           windowLenght > 0 &&
           initIndex + stride <= lengthOfTheChannel - windowLenght;
}

bool SlicingWindow::strideForward() {
    if (canStrideForward()) {
        initIndex += stride;
        return true;
    }
    return false;
}

bool SlicingWindow::itsInTheBorder() const {
    return !canStrideForward();
}

int SlicingWindow::getInitIndex() const {
    return initIndex;
}

int SlicingWindow::getWindowLenght() const {
    return windowLenght;
}

int SlicingWindow::getStride() const {
    return stride;
}

double SlicingWindow::calcWindowInSecs() const {
    if (channelAsocied == nullptr) {
        return 0.0;
    }
    return static_cast<double>(windowLenght) / channelAsocied->getSamplingRate();
}

double SlicingWindow::calcStartTimeInSecs() const {
    if (channelAsocied == nullptr) {
        return 0.0;
    }
    return channelAsocied->getMetadata().ConvertIndexSecs(initIndex);
}

double SlicingWindow::maxWindow(const std::string& selectionOfArr) const {
    if (channelAsocied == nullptr) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return channelAsocied->subRangeMaximum(initIndex, windowLenght, selectionOfArr);
}

double SlicingWindow::minWindow(const std::string& selectionOfArr) const {
    if (channelAsocied == nullptr) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return channelAsocied->subRangeMinimum(initIndex, windowLenght, selectionOfArr);
}

double SlicingWindow::energyWindow(const std::string& selectionOfArr) const {
    if (channelAsocied == nullptr) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return channelAsocied->subRangeEnergy(initIndex, windowLenght, selectionOfArr);
}

double SlicingWindow::averageWindow(const std::string& selectionOfArr) const {
    if (channelAsocied == nullptr) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return channelAsocied->subRangeAverage(initIndex, windowLenght, selectionOfArr);
}
