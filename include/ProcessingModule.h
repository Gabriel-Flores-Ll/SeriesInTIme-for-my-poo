#ifndef PROCESSING_MODULE_H
#define PROCESSING_MODULE_H

#include <iosfwd>
#include <string>

/**
 * @class ProcessingModule
 * @brief Abstract base class (Level 0) of the signal processing hierarchy.
 *        Defines the polymorphic contract for causal sample processing, state reset,
 *        deep cloning (Prototype), and lossless parameter serialization.
 *        Protected copy operations prevent object slicing while enabling clone() in subclasses.
 * @invariant !name.empty() and name contains no serialization delimiters (';', '|', '\n', '\r').
 */
class ProcessingModule {
private:
    std::string name;
    bool active;

    static std::string sanitizeName(const std::string& givenName);

protected:
    ProcessingModule(const ProcessingModule& other) = default;
    ProcessingModule& operator=(const ProcessingModule& other) = default;

public:
    /**
     * @brief Constructs an active processing module with a sanitized non-empty name.
     * @param givenName Descriptive module name (fallback: "Unnamed" if empty or whitespace-only).
     */
    explicit ProcessingModule(const std::string& givenName);

    /**
     * @brief Virtual destructor ensuring complete polymorphic cleanup of derived buffers.
     */
    virtual ~ProcessingModule() = default;

    const std::string& getName() const;
    bool isActive() const;
    void setActive(bool givenState);

    /**
     * @brief Processes a single input sample (or passes it through untouched when inactive).
     * @param givenSample Input sample value.
     * @return Filtered sample, or givenSample if inactive.
     */
    virtual double processSample(double givenSample) = 0;

    /**
     * @brief Clears internal filter state so the module can be reused cleanly on another channel.
     */
    virtual void reset() = 0;

    /**
     * @brief Allocates and returns a polymorphic deep copy of this module.
     * @return Newly heap-allocated ProcessingModule* owned by the caller.
     */
    virtual ProcessingModule* clone() const = 0;

    /**
     * @brief Serializes the module type, parameters, and active state for persistence.
     * @return Canonical key-value configuration string (e.g., "MEDIAN;W=5;active=1").
     */
    virtual std::string serialize() const = 0;

    /**
     * @brief Writes the serialized configuration of module to the output stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const ProcessingModule& module);
};

#endif
