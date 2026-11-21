#ifndef METADATA_SENSOR_H
#define METADATA_SENSOR_H

#include <string>

/**
 * @class MetadataSensor
 * @brief Stores acquisition metadata of a signal channel (identifier, unit, and sampling rate).
 * @invariant codeOfChannel is non-empty and contains no whitespace characters.
 * @invariant unitOfMeasure is non-empty and contains no whitespace characters.
 * @invariant samplingRate > 0.0 (expressed in Hz).
 */
class MetadataSensor {
private:
    std::string codeOfChannel;
    std::string unitOfMeasure;
    double samplingRate;

    /**
     * @brief Validates that a string has no whitespace and meets a minimum character length.
     * @param givenSTR String to validate (passed by const reference).
     * @param requiredContinuity Minimum number of non-whitespace characters required (>= 1).
     * @return true if the string contains no whitespace and its length >= requiredContinuity; false otherwise.
     */
    bool isAContinousSTR(const std::string& givenSTR, int requiredContinuity) const;

public:
    /**
     * @brief Constructs a valid MetadataSensor, falling back to safe defaults if any parameter violates invariants.
     * @param givenCode Non-empty channel code without spaces (fallback: "UNKNOWN").
     * @param givenUnit Non-empty physical unit without spaces (fallback: "uV").
     * @param givenSR Sampling rate in Hz; must be strictly > 0.0 (fallback: 1.0).
     */
    MetadataSensor(const std::string& givenCode, const std::string& givenUnit, double givenSR);

    /**
     * @brief Updates the channel code if valid; otherwise preserves the previous value.
     * @param givenCode Candidate channel code (non-empty, no spaces).
     * @return true if updated, false if rejected.
     */
    bool setCodeOfChannel(const std::string& givenCode);

    /**
     * @brief Updates the physical unit of measure if valid; otherwise preserves the previous value.
     * @param givenUnit Candidate unit string (non-empty, no spaces).
     * @return true if updated, false if rejected.
     */
    bool setUnitOfMeasure(const std::string& givenUnit);

    /**
     * @brief Updates the sampling rate if strictly positive; otherwise preserves the previous value.
     * @param givenSR Candidate sampling rate in Hz (> 0.0).
     * @return true if updated, false if rejected.
     */
    bool setSamplingRate(double givenSR);

    const std::string& getCodeOfChannel() const;
    const std::string& getUnitOfMeasure() const;
    double getSamplingRate() const;

    /**
     * @brief Converts a timestamp in seconds to its corresponding sample index.
     * @param givenSecond Time offset in seconds (>= 0.0; negative values are clamped to 0).
     * @return Non-negative sample index floor(samplingRate * givenSecond).
     */
    int ConvertIndexSecs(double givenSecond) const;

    /**
     * @brief Converts a sample index to its corresponding timestamp in seconds.
     * @param givenIndex Sample index (>= 0; negative values return 0.0).
     * @return Timestamp in seconds (givenIndex / samplingRate).
     */
    double ConvertIndexSecs(int givenIndex) const;
};

#endif
