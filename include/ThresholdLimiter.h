#ifndef THRESHOLD_LIMITER_H
#define THRESHOLD_LIMITER_H

#include "PointTransformer.h"

#include <string>

/**
 * @class ThresholdLimiter
 * @brief Hard-clipping limiter clamping every sample into [lowerLimit, upperLimit]
 *        prior to applying the inherited scale factor. Follows the Rule of Zero.
 * @invariant std::isfinite(lowerLimit) && std::isfinite(upperLimit) && lowerLimit < upperLimit.
 * @invariant scaleFactor != 0.0 and finite (inherited from PointTransformer).
 */
class ThresholdLimiter : public PointTransformer {
private:
    double lowerLimit;
    double upperLimit;

protected:
    double transform(double givenSample) const override;

public:
    ThresholdLimiter(double givenMin = -1.0,
                     double givenMax = 1.0,
                     double givenScaleFactor = 1.0);

    double getLowerLimit() const;
    double getUpperLimit() const;

    /**
     * @brief Updates clipping bounds if both are finite and givenMin < givenMax;
     *        otherwise preserves previous bounds.
     * @return true if updated, false if rejected.
     */
    bool setLimits(double givenMin, double givenMax);

    ProcessingModule* clone() const override;
    std::string serialize() const override;
};

#endif
