#ifndef POINT_TRANSFORMER_H
#define POINT_TRANSFORMER_H

#include "ProcessingModule.h"

#include <string>

/**
 * @class PointTransformer
 * @brief Intermediate abstract class (Level 1) for memoryless sample-by-sample operators
 *        followed by a validated non-zero finite scale factor (Template Method via transform()).
 *        Follows the Rule of Zero; protected copy operations prevent slicing.
 * @invariant std::isfinite(scaleFactor) && scaleFactor != 0.0.
 */
class PointTransformer : public ProcessingModule {
private:
    double scaleFactor;

protected:
    PointTransformer(const std::string& givenName, double givenFactor = 1.0);
    PointTransformer(const PointTransformer& other) = default;
    PointTransformer& operator=(const PointTransformer& other) = default;

    /**
     * @brief Concrete point-wise transformation applied before multiplying by scaleFactor.
     */
    virtual double transform(double givenSample) const = 0;

public:
    double getScaleFactor() const;

    /**
     * @brief Updates scaleFactor if finite and non-zero; otherwise preserves the previous valid value.
     *        Virtual so subclasses with stricter invariants (e.g., WaveRectifier > 0.0) can enforce them.
     * @return true if updated, false if rejected.
     */
    virtual bool setScaleFactor(double givenFactor);

    double processSample(double givenSample) override;
    void reset() override;
};

#endif
