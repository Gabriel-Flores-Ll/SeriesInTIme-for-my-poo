#include "SlicingWindow.h"

SlicingWindow::SlicingWindow(ChannelSignal* channelProvided, int windowLenghtProvided, int strideProvided) : channelAsocied(channelProvided), initIndex(0), lengthOfTheChannel(channelProvided->getNumberOfSamples()) {
    if (windowLenghtProvided >= 1 && strideProvided >= 1) {windowLenght = windowLenghtProvided; stride = strideProvided;}
}

void SlicingWindow::resetInitIndex() {initIndex = 0;}

void SlicingWindow::setInitIndex(int indexProvided) {if (indexProvided >= 0 && indexProvided + windowLenght >= lengthOfTheChannel - 1) {initIndex = indexProvided;}}

void SlicingWindow::strideForward() {if (initIndex + windowLenght + stride >= lengthOfTheChannel -1) {initIndex += stride;}}

bool SlicingWindow::itsInTheBorder() {if(initIndex + windowLenght == lengthOfTheChannel -1) {return true;} return false;}

double SlicingWindow::calcWindowInSecs () {return windowLenght/channelAsocied->getSamplingRate();}

double SlicingWindow::maxWindow(std::string selectionOfArr) {return channelAsocied->subRangemaximum(initIndex, windowLenght, selectionOfArr);}
double SlicingWindow::minWindow(std::string selectionOfArr) {return channelAsocied->subRangeminimum(initIndex, windowLenght, selectionOfArr);}
double SlicingWindow::energyWindow(std::string selectionOfArr) {return channelAsocied->subRangeEnergy(initIndex, windowLenght, selectionOfArr);}
double SlicingWindow::averageWindow(std::string selectionOfArr) {return channelAsocied ->subRangeAverage(initIndex, windowLenght, selectionOfArr);}
