#ifndef DIFFERENTIAL_FILTER_H
#define DIFFERENTIAL_FILTER_H

#include "WindowFilter.h"

/**
* @class DifferentialFilter
* @brief Teager-Kaiser Energy Operator (TKEO): psi = x[n-1]^2 - x[n-2] * x[n]. It uses the 3 most recent samples
*        and highlights sudden changes. With less than 3 samples it uses the first order finite difference x[n] - x[n-1]
*        (0.0 if there is only one sample).
* @invariant windowSize >= 3 and windowSize is odd (inherited from WindowFilter)
*/
class DifferentialFilter : public WindowFilter {
    protected:
        /**
        * @brief calculates the TKEO with the 3 newest samples in strict chronological order
        * @return the TKEO value, the finite difference if there are only 2 samples, or 0.0 if there is only 1
        */
        double calculate() const override;
    public:
        /**
        * @brief constructor of the differential filter
        * @param givenWindow size of the window, it is corrected to an odd value >= 3
        */
        explicit DifferentialFilter(int givenWindow = 3);

        /**
        * @brief deep copy of this filter
        * @return a pointer to a new DifferentialFilter in the heap, the caller must delete it
        */
        ProcessingModule* clone() const override;

        /**
        * @brief builds the configuration text of the filter
        * @return a text like "DIFFERENTIAL_TKEO;W=3"
        */
        std::string serialize() const override;
};

#endif
