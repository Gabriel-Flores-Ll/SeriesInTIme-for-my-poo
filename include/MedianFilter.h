#ifndef MEDIAN_FILTER_H
#define MEDIAN_FILTER_H

#include "WindowFilter.h"

#include <string>

/**
 * @class MedianFilter
 * @brief Non-linear impulse-noise filter computing the statistical median over the active window.
 *        Sorts a temporary copy via pointer-arithmetic Insertion Sort without modifying
 *        the inherited circular buffer. Uses a stack fast-path for windows <= 64 to avoid
 *        per-sample heap allocation overhead, with heap fallback for larger windows.
 *        Follows the Rule of Zero.
 * @invariant windowSize >= 3 and odd (inherited from WindowFilter).
 */
class MedianFilter : public WindowFilter {
private:
    static constexpr int kStackFastPathLimit = 64;

    static void insertionSort(double* givenArray, int givenQuantity);

protected:
    double calculate() const override;

public:
    explicit MedianFilter(int givenWindow = 5);

    ProcessingModule* clone() const override;
    std::string serialize() const override;
};

#endif
