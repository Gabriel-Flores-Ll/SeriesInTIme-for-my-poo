#ifndef WAVE_RECTIFIER_H
#define WAVE_RECTIFIER_H

#include "PointTransformer.h"

/** @brief working modes of the rectifier */
enum class RectifierMode {FullWave, HalfWave};

/**
* @class WaveRectifier
* @brief Full wave: |x|. Half wave: negative values become 0. It always gives values >= 0 (while the scale factor is positive).
* @invariant scaleFactor != 0.0 (inherited from PointTransformer)
*/
class WaveRectifier : public PointTransformer {
    private:
        RectifierMode mode;
    protected:
        /**
        * @brief rectifies one sample according to the current mode
        * @param givenSample the sample to be rectified
        * @return the absolute value in full wave mode, or the sample / 0.0 in half wave mode
        */
        double transform(double givenSample) const override;
    public:
        /**
        * @brief constructor of the rectifier
        * @param givenMode working mode, full wave by default
        */
        explicit WaveRectifier(RectifierMode givenMode = RectifierMode::FullWave);

        /**
        * @brief gives the current working mode
        * @return the mode of the rectifier
        */
        RectifierMode getMode() const;

        /**
        * @brief changes the working mode
        * @param givenMode the new mode
        */
        void setMode(RectifierMode givenMode);

        /**
        * @brief deep copy of this rectifier
        * @return a pointer to a new WaveRectifier in the heap, the caller must delete it
        */
        ProcessingModule* clone() const override;

        /**
        * @brief builds the configuration text of the rectifier
        * @return a text like "RECTIFIER;mode=FULL_WAVE" or "RECTIFIER;mode=HALF_WAVE"
        */
        std::string serialize() const override;
};

#endif
