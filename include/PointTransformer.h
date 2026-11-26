#ifndef POINT_TRANSFORMER_H
#define POINT_TRANSFORMER_H

#include "ProcessingModule.h"

/**
* @class PointTransformer
* @brief Intermediate abstract class (Level 1). It transforms every sample independently (no history).
*        The child defines "transform" and this class multiplies the result by the scale factor.
* @invariant scaleFactor != 0.0
*/
class PointTransformer : public ProcessingModule {
    private:
        double scaleFactor;
    protected:
        /**
        * @brief constructor of the PointTransformer
        * @param givenName descriptive name of the module, it must not be empty
        * @param givenFactor scale factor, if it is 0.0 it is forced to 1.0
        */
        PointTransformer(const std::string& givenName, double givenFactor);

        /**
        * @brief transformation of ONE sample, it is implemented by each leaf class
        * @param givenSample the sample to be transformed
        * @return the transformed sample, before applying the scale factor
        */
        virtual double transform(double givenSample) const = 0;
    public:
        /**
        * @brief gives the current scale factor
        * @return the scale factor, it is never 0.0
        */
        double getScaleFactor() const;

        /**
        * @brief changes the scale factor
        * @param givenFactor the new factor, it must be different from 0.0
        * @return true if the value was accepted, false if it was 0.0 (in that case the factor is forced to 1.0)
        */
        bool setScaleFactor(double givenFactor);

        /**
        * @brief bypass if inactive, otherwise it applies transform and then multiplies by the scale factor
        * @param givenSample the sample to be processed
        * @return transform(givenSample) * scaleFactor, or the same sample if the module is inactive
        */
        double processSample(double givenSample) override;

        /**
        * @brief does nothing, a point transformer has no history to clear
        */
        void reset() override;
};

#endif
