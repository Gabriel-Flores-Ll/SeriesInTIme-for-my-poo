#ifndef MOVING_AVERAGE_FILTER_H
#define MOVING_AVERAGE_FILTER_H

#include "WindowFilter.h"

/**
* @class MovingAverageFilter
* @brief Averages the samples of the window (low-pass filter). It reduces white noise, but it spreads isolated spikes.
* @invariant windowSize >= 3 and windowSize is odd (inherited from WindowFilter)
*/
class MovingAverageFilter : public WindowFilter {
    protected:
        /**
        * @brief calculates the arithmetic mean of the valid samples of the window (warm-up safe)
        * @return the average of the samples currently inside the buffer
        */
        double calculate() const override;
    public:
        /**
        * @brief constructor of the moving average filter
        * @param givenWindow size of the window, it is corrected to an odd value >= 3
        */
        explicit MovingAverageFilter(int givenWindow = 5);

        /**
        * @brief deep copy of this filter
        * @return a pointer to a new MovingAverageFilter in the heap, the caller must delete it
        */
        ProcessingModule* clone() const override;

        /**
        * @brief builds the configuration text of the filter
        * @return a text like "MOVING_AVERAGE;W=5"
        */
        std::string serialize() const override;
};

#endif
