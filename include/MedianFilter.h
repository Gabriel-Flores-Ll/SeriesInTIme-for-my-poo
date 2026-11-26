#ifndef MEDIAN_FILTER_H
#define MEDIAN_FILTER_H

#include "WindowFilter.h"

/**
* @class MedianFilter
* @brief Gives the median of the window. It removes isolated spikes without deforming the edges.
*        IMPORTANT: it sorts a temporary COPY, the inherited circular buffer is never sorted in place.
* @invariant windowSize >= 3 and windowSize is odd (inherited from WindowFilter)
*/
class MedianFilter : public WindowFilter {
    private:
        /**
        * @brief manual Insertion Sort (ascending) over an array, using pointer arithmetic
        * @param givenArray pointer to the first element of the array to be sorted
        * @param givenQuantity number of elements of the array
        */
        static void insertionSort(double* givenArray, int givenQuantity);
    protected:
        /**
        * @brief copies the valid samples to a temporary array, sorts that copy and takes the central element
        * @return the median of the samples currently inside the window (position (n - 1) / 2 of the sorted copy)
        */
        double calculate() const override;
    public:
        /**
        * @brief constructor of the median filter
        * @param givenWindow size of the window, it is corrected to an odd value >= 3
        */
        explicit MedianFilter(int givenWindow = 5);

        /**
        * @brief deep copy of this filter
        * @return a pointer to a new MedianFilter in the heap, the caller must delete it
        */
        ProcessingModule* clone() const override;

        /**
        * @brief builds the configuration text of the filter
        * @return a text like "MEDIAN;W=5"
        */
        std::string serialize() const override;
};

#endif
