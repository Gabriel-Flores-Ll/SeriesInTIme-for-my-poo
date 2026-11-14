#ifndef METADATA_SENSOR_H
#define METADATA_SENSOR_H

#include <string>


/** 
* @class MetadataSensor
* @brief It contains the data of the sensor like: code of the channel, 
* @invariant codeOfChannel != ""   
* @invariant unitOfMeasure != ""
* @invariant SamplingRate > 0.0
*/
class MetadataSensor {
    private:
        std::string codeOfChannel;
        std::string unitOfMeasure;
        double samplingRate;

        /** 
        * @brief Comprove if a string have a certain contuinity of char in a row, also checks it to have no spaces
        * @param givenSTR string provided by reference to check the continuty of it
        * @param requiredContinuity indicates the required continuty, for example 2, that indicates the minimum chars that the string must have
        * @return indicates if the string have all the requirements specified
        */
        bool isAContinousSTR(std::string &givenSTR, int requiredContinuity) const;
    public:
        /** 
        * @brief constructor of the MetadataSensor
        * @param givenCode string of the code of the channel provided, it must not be empty
        * @param givenUnit string thatindicates unit of measure, (micro volts or somthing), must be not empty
        * @param givenSR double that indicates the frecuency of the samples, it is given allways in Hz, SR(sampling rate), must be a valour more than 0
        */
        MetadataSensor(std::string givenCode, std::string givenUnit, double givenSR);

        /**
        * @brief This funtion change the name of the channel, changing the code
        * @param givenCode this is the code that will be the new for the channel must be diferrent from an empty code, at least 1 char code in a row
        * @return will return a bool to indicate if the code was changed
        */
        bool setCodeOfChannel (std::string givenCode);
        
        /**
        * @brief this funtion set the unit of measure of the sensor, in the case that it need to be changed
        * @param givenUnit this is the unit in that the measures were taken, it could be anything for now, just it need to be different from empty and have at least 2 char in a row
        * @return will return a bool to indicate if the unit was changed
        */
        bool setUnitOfMeasure (std::string givenUnit);

        /**
        * @brief Set the sampling rate of the measures
        * @param givenSR given sampling rate of the measures, it must be in Hz, also must be different from 0.0
        * @return will return a bool to indicate if the sampling rate was changed
    
        */
        bool setSamplingRate (double givenSR); 

        std::string getCodeOfChanne();
        std::string getUnitOfMeasure();
        double getSamplingRate();

        /**
        * @brief recives a exact sec in the time in order to convert it to the exact index of the element that correspond to that sec
        * @param givenSecond recives a second in a double, it musk be more than 0.0 to be correct
        * @return the corresponding index that mach to the sample in the second provided
        */
        int ConvertIndexSecs (double givenSecond) const;

        /**
        * @brief recives an Index of a sample in order to convert it to the exact second of meassurement of that sample
        * @param givenIndex the index of a sample
        * @return the corresponding second of measurement of the sample that mach to the index provided
        */
        double ConvertIndexSecs (int givenIndex) const;
    };

#endif 