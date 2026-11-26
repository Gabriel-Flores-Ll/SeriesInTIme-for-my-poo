#ifndef THRESHOLD_LIMITER_H
#define THRESHOLD_LIMITER_H

#include "PointTransformer.h"

/**
* @class ThresholdLimiter
* @brief Hard clipping: cuts every sample to the range [lowerLimit, upperLimit].
* @invariant lowerLimit < upperLimit (strict)
* @invariant scaleFactor != 0.0 (inherited from PointTransformer)
*/
class ThresholdLimiter : public PointTransformer {
    private:
        double lowerLimit;
        double upperLimit;
    protected:
        /**
        * @brief cuts one sample to the allowed range
        * @param givenSample the sample to be limited
        * @return lowerLimit if the sample is below it, upperLimit if it is above, or the same sample if it is inside the range
        */
        double transform(double givenSample) const override;
    public:
        /**
        * @brief constructor of the limiter
        * @param givenMin lower limit
        * @param givenMax upper limit, it must be strictly greater than givenMin, otherwise the limits stay in the default [-1.0, 1.0]
        */
        ThresholdLimiter(double givenMin = -1.0, double givenMax = 1.0);

        /**
        * @brief gives the lower limit
        * @return the lower limit of the range
        */
        double getLowerLimit() const;

        /**
        * @brief gives the upper limit
        * @return the upper limit of the range
        */
        double getUpperLimit() const;

        /**
        * @brief changes both limits at the same time
        * @param givenMin new lower limit
        * @param givenMax new upper limit, it must be strictly greater than givenMin
        * @return true if the limits were applied, false if givenMin >= givenMax (the limits do not change)
        */
        bool setLimits(double givenMin, double givenMax);

        /**
        * @brief deep copy of this limiter
        * @return a pointer to a new ThresholdLimiter in the heap, the caller must delete it
        */
        ProcessingModule* clone() const override;

        /**
        * @brief builds the configuration text of the limiter
        * @return a text like "LIMITER;min=0.000000;max=100.000000"
        */
        std::string serialize() const override;
};

#endif
