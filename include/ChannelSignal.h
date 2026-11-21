#ifndef CHANNEL_SIGNAL_H
#define CHANNEL_SIGNAL_H

#include<string>

#include "DinamicArr.h"
#include "MetadataSensor.h"
#include "ChannelStadistics.h"

/**
* @class ChannelSignal
* @brief Manage the classes channelStadistics and MetadataSensor and two dinamic arrays (with raw and processed samples) in order to model a Signal of a channel, and its stadistics and data of this
* @invariant lenght of the samples (N) must be more than 2, this to have a defined variance
* @invariant the two pointers must be pointing to a valid block of memory of lenght N
* @invariant allways when the elements of the array or the entire array is changed dirtyFlag will indicate it
*/
class ChannelSignal {
    private:
        MetadataSensor nativeMetaData;
        ChannelStadistics nativeStadistics;
        DinamicArr rawSamples;
        DinamicArr processedSamples;
        int totalOfSamples;
        bool dirtyFlag;
    public:
        /**
        * @brief Parameter constructor of the class, copy all the necesary valours into the class and sets all the operations that are needed to make a valid object and sets the dirtyFlag to true
        * @param givenCode string of the code of the channel provided, it must not be empty
        * @param givenUnit string thatindicates unit of measure, (micro volts or somthing), must be not empty
        * @param givenSR double that indicates the frecuency of the samples, it is given allways in Hz, SR(sampling rate), must be a valour more than 0
        * @param inicialData recives the incial Data of streams to iniciate the native arrays
        * @param ProvidedNumOfElements its the number of elements that are in the incial Data
        */
        ChannelSignal(std::string givenCode, std::string givenUnit, double givenSR, double* inicialData, int ProvidedNumOfElements);

        /**
        * @brief the constructor by copy of the class, relied in the overcharge of =
        * @param other anotherSignal by reference
        */
        ChannelSignal(const ChannelSignal &other);
        
        /**
        * @brief The overcharge of the operator = to make a deep copy of the object
        * @param other another ChannelSignal by reference
        * @return A ChannelSignal that is a deep copy 
        */
        ChannelSignal operator=(const ChannelSignal &other);    

        /**
        * @brief Evoques the method of the subclass ChannelStadistics to update the stadistics and set to true the dirtyFlag
        */
        void updateInternalStadistics(std::string selectionOfArr);

        /**
        * @brief Brought the element of the raw/Processed solicited by valour, reliing in the methods of the arrays
        * @param index Its the index of the element that its beeing solicited
        * @return A double that is the element solicited
        */
        double getRawElement (int index);
        double getProcesedElement(int index);

        int getNumberOfSamples();
        double getSamplingRate();

        /**
        * @brief set an element in raw/processed in the respective array and set the dirty flag to false
        */
        bool setRawElement(double providedElement, int indexProvided);
        bool setPracessedElement(double providedElement, int indexProvided);

        /**
        * @brief Restores the array of the processed samples with the raw ones, this by making a copy
        */
        bool restoreProcessedSamples();

        /**
        * @brief Process all the metrics on a sub-range working in the array of selection
        * @param initOfRange indicates the inicial index of the range
        * @param endOfRange indicates the ending index of the range
        * @param selectionOfArr selects beethwin the two arrays raw and processed to do the calculus of the metrics 
        * @return the final calculus of the metrics
        */
        double subRangeAverage(int initOfRange, int numberOfElements, std::string selectionOfArr);  
        double subRangeminimum(int initOfRange, int numberOfElements, std::string selectionOfArr);
        double subRangemaximum(int initOfRange, int numberOfElements, std::string selectionOfArr);
        double subRangeEnergy(int initOfRange, int numberOfElements, std::string selectionOfArr);
};


#endif