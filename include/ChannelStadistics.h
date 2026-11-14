#ifndef CHANNEL_STADISTICS_H
#define CHANNEL_STADISTICS_H

/** 
* @class ChannelStadistics
* @brief It contains the stadistics of the channel associated, in order to have operations and stats about the data that is beeing analized.
* @invariant variation must be >= 0.0 , if the analized part have lenght = 1 it need to be defined explicitly to 0, to avoid divitions on zero
* @invariant standardDesviation variation must be >= 0.0, if the analized part have lenght = 1 it need to be defined explicitly to 0, to avoid divitions on zero
* @invariant minimum allways <= to maximun
* @invariant totalEnergy must be >= 0.0
*/
class ChannelStadistics {
    private:
        double average;
        double variation;
        double standardDesviation;  
        double minimum;
        double maximum;
        double totalEnergy;
    public:
        /** 
        * @brief constructor of the ChannelStadistics class, it will call the method updateStadistics in order to construct the first iteration of stadistics of the class
        * @param initOfMemory a pointer/direction of memory of the inicial element of the block of memory of the channel data
        * @param quantityOfElements indicates the number of elements that the channel posses
        */
        ChannelStadistics (double* initOfMemory, int quantityOfElements);
        
        /** 
        * @brief default constructor of the class, it sets all the valours to 0 and wait until the updateStadistics methotd to be run manually 
        */
        ChannelStadistics();

        /**
        * @brief 
        */
        bool updateStadistics();
};

#endif