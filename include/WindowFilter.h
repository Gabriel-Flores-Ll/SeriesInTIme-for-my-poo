#ifndef WINDOW_FILTER_H
#define WINDOW_FILTER_H

#include "ProcessingModule.h"

/**
* @class WindowFilter
* @brief Intermediate abstract class (Level 1). It manages its own CIRCULAR BUFFER in the heap with the last W samples.
*        It implements the shared logic (insert and reset) and delegates the math to the children through the pure virtual
*        method "calculate" (Template Method pattern).
* @invariant windowSize >= 3 and windowSize is odd
* @invariant 0 <= validSamples <= windowSize
* @invariant 0 <= writeIndex < windowSize
*/
class WindowFilter : public ProcessingModule {
    private:
        int windowSize;
        double* buffer;
        int writeIndex;     // position where the next sample will be written
        int validSamples;   // how many real samples are inside the buffer (warm-up phase)

        /**
        * @brief corrects the window size to the nearest valid odd value
        * @param givenWindow the requested window size
        * @return 3 if it is less than 3, givenWindow + 1 if it is even, or givenWindow itself if it is already valid
        */
        static int correctWindow(int givenWindow);

    protected:
        /**
        * @brief constructor of the WindowFilter, it reserves the circular buffer in the heap filled with zeros
        * @param givenName descriptive name of the module, it must not be empty
        * @param givenWindow requested window size, it is corrected to an odd value >= 3
        */
        WindowFilter(const std::string& givenName, int givenWindow);

        /**
        * @brief deep copy constructor (Rule of Three), it reserves its own buffer and copies the content
        * @param other the filter to be copied
        */
        WindowFilter(const WindowFilter& other);

        /**
        * @brief assignment operator (Rule of Three), protected against self-assignment
        * @param other the filter to be copied
        * @return a reference to this object
        */
        WindowFilter& operator=(const WindowFilter& other);

        /**
        * @brief Template Method: each child decides what to calculate with the samples of the buffer, it is called AFTER
        *        inserting the new sample, so validSamples >= 1
        * @return the value calculated over the samples of the window
        */
        virtual double calculate() const = 0;

        /**
        * @brief gives the number of real samples that are currently in the buffer
        * @return a number between 0 and windowSize
        */
        int getValidSamples() const;

        /**
        * @brief gives the start of the buffer, the positions [0, validSamples) are the valid ones (NOT in chronological order)
        * @return a pointer to the first position of the circular buffer
        */
        const double* getBuffer() const;

        /**
        * @brief gives the most recent sample (k = 0), the previous one (k = 1), etc. in chronological order using modular arithmetic
        * @param k how many samples back from the newest one, it must be in [0, validSamples)
        * @return the requested sample, or 0.0 if k is out of range
        */
        double recentSample(int k) const;

    public:
        /**
        * @brief destructor, it frees the circular buffer
        */
        ~WindowFilter() override;

        /**
        * @brief gives the size of the window
        * @return the window size W, always odd and >= 3
        */
        int getWindowSize() const;

        /**
        * @brief inserts the sample in the circular buffer and then calls calculate()
        * @param givenSample the sample to be processed
        * @return the value calculated by the concrete filter, or the same sample if the filter is inactive
        */
        double processSample(double givenSample) override;

        /**
        * @brief empties the buffer (validSamples = 0, writeIndex = 0)
        */
        void reset() override;
};

#endif
