#ifndef PROCESSING_PIPELINE_H
#define PROCESSING_PIPELINE_H

#include <iosfwd>
#include <string>

class ProcessingModule;

/**
 * @class ProcessingPipeline
 * @brief Manual RAII polymorphic chain owning a dynamic array of ProcessingModule stages
 *        (Rule of Three with strong exception safety). Decoupled from ChannelSignal;
 *        ChannelSignal applies pipelines cleanly via ChannelSignal::applyPipeline().
 * @invariant 0 <= count && count <= capacity.
 * @invariant If capacity == 0, stages == nullptr and count == 0.
 * @invariant For every 0 <= i < count, *(stages + i) != nullptr and all addresses are unique.
 */
class ProcessingPipeline {
private:
    static constexpr int kDefaultCapacity = 4;

    ProcessingModule** stages;
    int count;
    int capacity;

    void resize(int newCapacity);
    void freeStages();

public:
    /**
     * @brief Constructs an empty pipeline with the requested initial capacity.
     * @param initialCapacity Initial pointer array capacity (>= 0).
     * @throws std::invalid_argument if initialCapacity < 0.
     */
    explicit ProcessingPipeline(int initialCapacity = kDefaultCapacity);

    /**
     * @brief Deep-copy constructor (Rule of Three) cloning every stage with strong exception safety.
     */
    ProcessingPipeline(const ProcessingPipeline& other);

    /**
     * @brief Copy-assignment operator (Rule of Three) providing the strong exception guarantee.
     */
    ProcessingPipeline& operator=(const ProcessingPipeline& other);

    /**
     * @brief Destructor (Rule of Three) releasing all owned stages polymorphically.
     */
    ~ProcessingPipeline();

    /**
     * @brief Polymorphically clones givenModule and appends the copy to the pipeline.
     * @param givenModule Stage to clone.
     */
    void addStage(const ProcessingModule& givenModule);

    /**
     * @brief Transfers exclusive heap ownership of an already-allocated ProcessingModule into the pipeline.
     *        Deletes givenModule before rethrowing if internal reallocation fails.
     * @param givenModule Non-null heap-allocated module pointer not already owned by this pipeline.
     * @throws std::invalid_argument if givenModule == nullptr or already owned by this pipeline.
     */
    void adoptStage(ProcessingModule* givenModule);

    /**
     * @brief Deletes the stage at givenIndex, shifts subsequent stages left, and resets all
     *        remaining stages so no stale filter history leaks across the topology change.
     * @param givenIndex Zero-based stage index in [0, count - 1].
     * @return true if removed, false if index is out of range.
     */
    bool removeStage(int givenIndex);

    /**
     * @brief Deletes all owned stages and resets count to 0 while preserving allocated capacity.
     */
    void clear();

    /**
     * @brief Activates or deactivates (bypasses) the stage at givenIndex.
     * @param givenIndex Zero-based stage index in [0, count - 1].
     * @param givenState true to activate, false to bypass.
     * @return true if updated, false if index is out of range.
     */
    bool setStageActive(int givenIndex, bool givenState);

    /**
     * @brief Passes a single sample sequentially through all registered stages.
     * @param givenSample Input sample value.
     * @return Output sample after the final stage (or givenSample if empty).
     */
    double processSample(double givenSample);

    /**
     * @brief Clears internal history of every stage in the pipeline.
     */
    void reset();

    int getCount() const;
    int getCapacity() const;
    bool isEmpty() const;

    /**
     * @brief Bounds-checked read-only access to the stage at givenIndex.
     * @throws std::out_of_range if givenIndex < 0 or givenIndex >= count.
     */
    const ProcessingModule& getStage(int givenIndex) const;

    /**
     * @brief Bounds-checked read-only indexed access to the stage at givenIndex.
     * @throws std::out_of_range if givenIndex < 0 or givenIndex >= count.
     */
    const ProcessingModule& operator[](int givenIndex) const;

    /**
     * @brief Serializes all stages separated by " | " (empty string if pipeline is empty).
     */
    std::string serialize() const;

    bool operator==(const ProcessingPipeline& other) const;
    bool operator!=(const ProcessingPipeline& other) const;

    /**
     * @brief Writes a formatted summary of the pipeline stages to the output stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const ProcessingPipeline& pipeline);
};

#endif
