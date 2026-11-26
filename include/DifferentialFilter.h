#ifndef DIFFERENTIAL_FILTER_H
#define DIFFERENTIAL_FILTER_H

#include "WindowFilter.h"

#include <string>

/**
 * @class DifferentialFilter
 * @brief Causal Teager-Kaiser Energy Operator (TKEO): psi[n] = x[n-1]^2 - x[n-2] * x[n].
 *        Strictly operates on a fixed 3-sample neighborhood (W = 3, 1-sample causal latency).
 *        Returns 0.0 while validSamples < 3 to preserve dimensional consistency ([amplitude]^2).
 *        Follows the Rule of Zero.
 * @invariant windowSize == 3.
 */
class DifferentialFilter : public WindowFilter {
protected:
    double calculate() const override;

public:
    DifferentialFilter();

    ProcessingModule* clone() const override;
    std::string serialize() const override;
};

#endif
