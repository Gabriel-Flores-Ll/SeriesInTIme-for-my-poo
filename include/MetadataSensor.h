#ifndef METADATA_SENSOR_H
#define METADATA_SENSOR_H

#include <string>

/** 
* @class metadata_sensor
* @brief It contains the data of the sensor like: code of the channel, 
* @invariant codeOfChannel != ""
*/
class Metadata_sensor {
    private:
        std::string codeOfChannel;
    public:
        /** 
        * @brief construct the metadata sensor
        * @param givenCode string of the code of the channel provided, it must not be empty
        */
        Metadata_sensor(std::string givenCode);
};

#endif 