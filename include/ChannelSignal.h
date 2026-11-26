#ifndef CHANNEL_SIGNAL_H
#define CHANNEL_SIGNAL_H

#include <string>

#include "DinamicArr.h"
#include "MetadataSensor.h"
#include "ChannelStadistics.h"

class ProcessingModule;
class ProcessingPipeline;

/**
 * @class ChannelSignal
 * @brief Models a single time-series signal channel composing metadata, baseline statistics,
 *        an immutable raw sample buffer, and a mutable processed sample buffer (Rule of Zero via DinamicArr).
 * @invariant totalOfSamples >= 2 (guarantees well-defined Bessel variance).
 * @invariant rawSamples.getNumberOfElements() == totalOfSamples and processedSamples.getNumberOfElements() == totalOfSamples.
 * @invariant rawSamples remains immutable after construction so restoreProcessedSamples() can always revert filters.
 * @invariant statsDirty is true whenever processedSamples has changed since the last statistics recomputation.
 */
class ChannelSignal {
private:
    MetadataSensor nativeMetaData;
    mutable ChannelStadistics nativeStadistics;
    DinamicArr rawSamples;
    DinamicArr processedSamples;
    int totalOfSamples;
    mutable bool statsDirty;

    /**
     * @brief Selects the internal buffer matching "raw" or "processed".
     * @return Const pointer to the selected DinamicArr, or nullptr if selectionOfArr is invalid.
     */
    const DinamicArr* selectBuffer(const std::string& selectionOfArr) const;

    /**
     * @brief Validates that [initOfRange, initOfRange + numberOfElements) lies completely within [0, totalOfSamples).
     */
    bool isValidSubRange(int initOfRange, int numberOfElements) const;

public:
    /**
     * @brief Constructs a valid ChannelSignal with deep-copied raw and processed buffers.
     * @param givenCode Non-empty channel identifier without spaces.
     * @param givenUnit Non-empty unit of measure without spaces.
     * @param givenSR Sampling rate in Hz (> 0.0).
     * @param inicialData Pointer to initial sample values (non-null).
     * @param providedNumOfElements Number of samples (>= 2). If invalid, initializes a safe 2-sample zero signal.
     */
    ChannelSignal(const std::string& givenCode,
                  const std::string& givenUnit,
                  double givenSR,
                  const double* inicialData,
                  int providedNumOfElements);

    /**
     * @brief Returns baseline statistics synchronized with the latest processedSamples state (lazy evaluation).
     * @return Const reference to the composed ChannelStadistics object (zero heap allocation).
     */
    const ChannelStadistics& getProcessedStatistics() const;

    /**
     * @brief Provides read-only access to the sensor metadata.
     */
    const MetadataSensor& getMetadata() const;

    /**
     * @brief Retrieves a sample from the immutable raw buffer.
     * @param index Position in [0, totalOfSamples).
     * @return Sample value, or quiet_NaN() if out of bounds.
     */
    double getRawElement(int index) const;

    /**
     * @brief Retrieves a sample from the processed buffer.
     * @param index Position in [0, totalOfSamples).
     * @return Sample value, or quiet_NaN() if out of bounds.
     */
    double getProcessedElement(int index) const;

    int getNumberOfSamples() const;
    double getSamplingRate() const;

    /**
     * @brief Modifies a sample in processedSamples and marks cached statistics as dirty only if the write succeeds.
     * @param providedElement New processed value.
     * @param indexProvided Position in [0, totalOfSamples).
     * @return true if updated, false if index was out of bounds.
     */
    bool setProcessedElement(double providedElement, int indexProvided);

    /**
     * @brief Restores processedSamples to an exact deep copy of rawSamples and invalidates cached statistics.
     */
    void restoreProcessedSamples();

    /**
     * @brief Resets module, traverses the selected buffer using pointer arithmetic, writes filtered samples
     *        into processedSamples (leaving rawSamples intact), and marks cached statistics as dirty.
     * @param module Polymorphic processing module applied to each sample.
     * @param sourceBuffer Source buffer selector: "processed" (default, allows chaining) or "raw".
     * @return true if applied, false if sourceBuffer is invalid (signal left unchanged).
     */
    bool applyModule(ProcessingModule& module, const std::string& sourceBuffer = "processed");

    /**
     * @brief Resets pipeline, traverses the selected buffer using pointer arithmetic, writes filtered samples
     *        into processedSamples (leaving rawSamples intact), and marks cached statistics as dirty.
     * @param pipeline Polymorphic processing pipeline applied in cascade to each sample.
     * @param sourceBuffer Source buffer selector: "processed" (default) or "raw".
     * @return true if applied, false if sourceBuffer is invalid (signal left unchanged).
     */
    bool applyPipeline(ProcessingPipeline& pipeline, const std::string& sourceBuffer = "processed");

    /**
     * @brief Computes statistical metrics over a contiguous sub-range [initOfRange, initOfRange + numberOfElements).
     * @param initOfRange Starting sample index (>= 0).
     * @param numberOfElements Number of samples in the sub-range (> 0).
     * @param selectionOfArr Buffer selector: "raw" or "processed".
     * @return Computed metric, or quiet_NaN() if range or buffer selector is invalid.
     */
    double subRangeAverage(int initOfRange, int numberOfElements, const std::string& selectionOfArr) const;
    double subRangeMinimum(int initOfRange, int numberOfElements, const std::string& selectionOfArr) const;
    double subRangeMaximum(int initOfRange, int numberOfElements, const std::string& selectionOfArr) const;
    double subRangeEnergy(int initOfRange, int numberOfElements, const std::string& selectionOfArr) const;
};

#endif