#ifndef WINDOW_FILTER_H
#define WINDOW_FILTER_H

#include "ProcessingModule.h"

#include <string>

/**
 * @class WindowFilter
 * @brief Intermediate abstract class (Level 1) managing an RAII circular buffer in the heap
 *        for the last W samples (Rule of Three with strong exception safety).
 *        Implements Template Method via calculate().
 * @invariant kMinWindowSize <= windowSize && windowSize <= kMaxWindowSize && windowSize % 2 == 1.
 * @invariant buffer != nullptr.
 * @invariant 0 <= validSamples && validSamples <= windowSize.
 * @invariant 0 <= writeIndex && writeIndex < windowSize.
 */
class WindowFilter : public ProcessingModule {
private:
    static constexpr int kMinWindowSize = 3;
    static constexpr int kMaxWindowSize = 100001;

    int windowSize;
    double* buffer;
    int writeIndex;
    int validSamples;

    static int correctWindow(int givenWindow);

protected:
    /**
     * @brief Constructs a WindowFilter with a zero-initialized circular buffer of odd size in [3, 100001].
     * @param givenName Module identifier.
     * @param givenWindow Requested window width (clamped to [3, 100001], incremented by 1 if even).
     */
    WindowFilter(const std::string& givenName, int givenWindow);

    /**
     * @brief Deep-copy constructor (Rule of Three) allocating an independent circular buffer.
     */
    WindowFilter(const WindowFilter& other);

    /**
     * @brief Copy-assignment operator (Rule of Three) providing the strong exception guarantee.
     */
    WindowFilter& operator=(const WindowFilter& other);

    /**
     * @brief Template Method hook invoked after inserting a finite sample (validSamples >= 1).
     * @return Filtered output computed from the current circular buffer contents.
     */
    virtual double calculate() const = 0;

    int getValidSamples() const;

    /**
     * @brief Returns read-only pointer to the backing buffer; indices [0, validSamples) contain
     *        valid samples (not necessarily in chronological order).
     */
    const double* getBuffer() const;

    /**
     * @brief Retrieves the k-th most recent sample in chronological order (k = 0 is newest).
     * @param k Lookback offset in [0, validSamples).
     * @return Sample value, or 0.0 if k is out of range.
     */
    double recentSample(int k) const;

public:
    ~WindowFilter() override;

    int getWindowSize() const;

    /**
     * @brief Inserts a finite sample into the circular buffer and returns calculate().
     *        Returns givenSample unchanged if inactive or non-finite (preventing circular buffer poisoning).
     */
    double processSample(double givenSample) override;

    void reset() override;
};

#endif
