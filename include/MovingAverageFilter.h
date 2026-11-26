#ifndef MOVING_AVERAGE_FILTER_H
#define MOVING_AVERAGE_FILTER_H

#include "WindowFilter.h"

#include <string>

/**
 * @class MovingAverageFilter
 * @brief Causal FIR low-pass moving average filter over the active window samples.
 *        Follows the Rule of Zero (circular buffer owned by WindowFilter).
 * @invariant windowSize >= 3 and odd (inherited from WindowFilter).
 */
class MovingAverageFilter : public WindowFilter {
protected:
    double calculate() const override;

public:
    explicit MovingAverageFilter(int givenWindow = 5);

    ProcessingModule* clone() const override;
    std::string serialize() const override;
};

#endif
