#ifndef WAVE_RECTIFIER_H
#define WAVE_RECTIFIER_H

#include "PointTransformer.h"

#include <string>

/**
 * @enum RectifierMode
 * @brief Operating modes for WaveRectifier.
 */
enum class RectifierMode {
    FullWave,
    HalfWave
};

/**
 * @class WaveRectifier
 * @brief Full-wave (|x|) or half-wave (max(0, x)) rectifier enforcing strictly positive scaleFactor
 *        so active output is guaranteed non-negative for all finite samples.
 *        Follows the Rule of Zero.
 * @invariant std::isfinite(getScaleFactor()) && getScaleFactor() > 0.0.
 */
class WaveRectifier : public PointTransformer {
private:
    RectifierMode mode;

protected:
    double transform(double givenSample) const override;

public:
    /**
     * @brief Constructs a WaveRectifier with strictly positive gain (fallback: 1.0 if <= 0.0 or non-finite).
     */
    explicit WaveRectifier(RectifierMode givenMode = RectifierMode::FullWave,
                           double givenScaleFactor = 1.0);

    RectifierMode getMode() const;
    void setMode(RectifierMode givenMode);

    /**
     * @brief Updates scaleFactor only if finite and strictly > 0.0; preserves previous value otherwise.
     * @return true if updated, false if rejected.
     */
    bool setScaleFactor(double givenFactor) override;

    ProcessingModule* clone() const override;
    std::string serialize() const override;
};

#endif
