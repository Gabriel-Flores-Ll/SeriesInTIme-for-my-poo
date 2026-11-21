#ifndef SLICING_WINDOW_H
#define SLICING_WINDOW_H

class ChannelSignal;

/**
* @class SlicingWindow 
* @brief its and object that uses a pointer to a ChannelSignal in order to search around the array of this one and calculate certain stadistics in this windows 
* @invariant When uses the methods  of the class it is needed to have an ChannelSignal object asigned to the pointer in the class
* @invariant the lenght of the window (W) must be at least 1
* @invariant The stride must be  <= 1
* @invariant The adition of the actual index and the lenght of the Channel (W) would never exeed the total cuantity of the samples of the asociated Channel
*/
class SlicingWindow {
    private:
        ChannelSignal * const channelAsocied;
        int initIndex;
        int windowLenght;
        int stride;
    public:
        /**
        * @brief Constructor with parameters of the class it assing all the fundamental variables of the class to make correctly a valid object
        * @param channelProvided recives the channel that will be assigned to the slicing window, it is a constand 
        * @param windowLenghtProvided an integer that is the lenght of the window and the show of elements that will have
        * @param strideProvided its the number of steps that the window will move in each stride 
        */
        SlicingWindow(ChannelSignal* channelProvided, int windowLenghtProvided, int strideProvided);

        /**
        * @brief Resets the init index of the slicing window to the inicial position
        */
        void resetInitIndex();

        /**
        * @brief Sets the index to a determinate position to inspect
        * @param indexProvided the index provided to set position
        */
        void setInitIndex(int indexProvided);

        /**
        * @brief Advances a strice to continue with the revition of the array, it need to comprove the invariants, specially not overpass the total cuantity of the samples in the channel
        */
        void strideForward();

        /**
        * @brief Comproves if the window reaches the border of the array, adding the leght and the actual index
        * @return a bool of the state of the window, if reaches(True) in other case (False)
        */
        bool itsInTheBorder();

        /**
        * @brief it calcs the window in secs with the frecuency of the samples
        * @return The final calc in seconds of the window
        */
        double calcWindowInSecs ();

        /** 
        * @brief Delegates the funcions of the calculus of the stadistics in the window to the channel funcions
        * @return the results of the calculus of stadistics that the channel provides, and passes it to the caller
        */
        double maxWindow();
        double minWindow();
        double energyWindow();
        double averageWindow();
};

#endif 