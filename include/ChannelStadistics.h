#ifndef CHANNEL_STADISTICS_H
#define CHANNEL_STADISTICS_H

/**
 * @class ChannelStadistics
 * @brief Computes and stores stationary baseline statistics of a signal channel using a numerically stable two-pass algorithm.
 * @invariant variation >= 0.0 (explicitly 0.0 when N == 1).
 * @invariant standardDesviation >= 0.0 (sqrt(variation)).
 * @invariant minimum <= maximum.
 * @invariant totalEnergy >= 0.0.
 */
class ChannelStadistics {
private:
    double average;
    double variation;
    double standardDesviation;
    double minimum;
    double maximum;
    double totalEnergy;

public:
    /**
     * @brief Default constructor initializing all statistical metrics to 0.0.
     */
    ChannelStadistics();

    /**
     * @brief Parameterized constructor initializing metrics to 0.0 and immediately computing baseline statistics.
     * @param initOfMemory Read-only pointer to the first sample in memory.
     * @param quantityOfElements Number of samples (> 0). If invalid, metrics remain safely at 0.0.
     */
    ChannelStadistics(const double* initOfMemory, int quantityOfElements);

    /**
     * @brief Recomputes all baseline statistics in two passes using pointer arithmetic.
     * @param initOfMemory Read-only pointer to the first sample (must be non-null).
     * @param quantityOfElements Number of samples (must be > 0).
     * @return true if computation succeeded, false if arguments were invalid (previous state preserved).
     */
    bool updateStadistics(const double* initOfMemory, int quantityOfElements);

    double getAverage() const;
    double getVariation() const;
    double getStandardDesviation() const;
    double getMinimum() const;
    double getMaximum() const;
    double getTotalEnergy() const;
};

#endif
