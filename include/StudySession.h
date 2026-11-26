#ifndef STUDY_SESSION_H
#define STUDY_SESSION_H

#include "ChannelSignal.h"
#include "DetectionEngine.h"
#include "EventRegistry.h"
#include "ProcessingPipeline.h"

#include <iosfwd>
#include <string>

class EventComparator;

/**
 * @class StudySession
 * @brief Top-level domain orchestrator managing a multi-channel analytical study.
 *        Composes ProcessingPipeline, DetectionEngine, and EventRegistry strictly by value,
 *        and owns a manual RAII dynamic array of ChannelSignal* channels (Rule of Three with
 *        strong exception safety, zero STL containers).
 * @invariant !sessionName.empty() and sessionName contains no newline or ';' delimiters.
 * @invariant 0 <= channelCount && channelCount <= channelCapacity.
 * @invariant If channelCapacity == 0, channels == nullptr and channelCount == 0.
 * @invariant For every 0 <= i < channelCount, *(channels + i) != nullptr and all addresses are unique.
 */
class StudySession {
private:
    static constexpr int kDefaultChannelCapacity = 4;

    std::string sessionName;
    ChannelSignal** channels;
    int channelCount;
    int channelCapacity;

    ProcessingPipeline pipeline;
    DetectionEngine engine;
    EventRegistry registry;

    static std::string sanitizeSessionName(const std::string& givenName);

    void resizeChannels(int newCapacity);
    void releaseChannels();

public:
    /**
     * @brief Constructs an empty StudySession with validated session name and initial channel capacity.
     * @param givenName Descriptive session identifier (fallback: "UntitledSession" if empty/whitespace).
     * @param initialChannelCapacity Initial pointer capacity for channels (>= 0).
     * @throws std::invalid_argument if initialChannelCapacity < 0.
     */
    explicit StudySession(const std::string& givenName = "UntitledSession",
                          int initialChannelCapacity = kDefaultChannelCapacity);

    /**
     * @brief Deep-copy constructor (Rule of Three) duplicating all channels, pipeline stages,
     *        detectors, and events with strong exception safety.
     */
    StudySession(const StudySession& other);

    /**
     * @brief Copy-assignment operator (Rule of Three) with self-assignment guard and leak-free cleanup.
     */
    StudySession& operator=(const StudySession& other);

    /**
     * @brief Destructor (Rule of Three) releasing all owned ChannelSignal instances and array buffer.
     *        Composed members (pipeline, engine, registry) are destroyed automatically in place.
     */
    ~StudySession();

    const std::string& getSessionName() const;
    void setSessionName(const std::string& givenName);

    void addChannel(const ChannelSignal& signal);
    void adoptChannel(ChannelSignal* signal);
    bool removeChannel(int index);
    void clearChannels();

    int getChannelCount() const;
    int getChannelCapacity() const;
    bool hasChannels() const;

    const ChannelSignal& getChannel(int index) const;
    ChannelSignal& getChannel(int index);

    /**
     * @brief Searches for the first channel whose MetadataSensor code matches givenCode.
     * @return Zero-based channel index if found, or -1 if not present.
     */
    int findChannelByCode(const std::string& givenCode) const;

    ProcessingPipeline& getPipeline();
    const ProcessingPipeline& getPipeline() const;

    DetectionEngine& getEngine();
    const DetectionEngine& getEngine() const;

    EventRegistry& getRegistry();
    const EventRegistry& getRegistry() const;

    /**
     * @brief Applies the composed ProcessingPipeline to a single channel.
     * @param index Zero-based channel index.
     * @param sourceBuffer Source buffer selector ("raw" or "processed").
     * @return true if applied, false if index or sourceBuffer is invalid.
     */
    bool applyPipelineToChannel(int index, const std::string& sourceBuffer = "raw");

    /**
     * @brief Applies the composed ProcessingPipeline sequentially to all channels in the session.
     * @param sourceBuffer Source buffer selector ("raw" or "processed").
     * @return true if applied to all channels (including 0 channels), false if sourceBuffer is invalid.
     */
    bool applyPipelineToAll(const std::string& sourceBuffer = "raw");

    /**
     * @brief Restores processedSamples to rawSamples on every channel in the session.
     */
    void restoreAllChannels();

    /**
     * @brief Executes the composed DetectionEngine across all registered channels,
     *        replaces the composed EventRegistry with the consolidated findings, and sorts them.
     * @param sortBySeverity If true, triages descending by severity via EventSorter::quickSort;
     *                       if false, orders chronologically via EventSorter::quickSort.
     * @return Total number of events detected across all channels.
     */
    int runAnalysis(bool sortBySeverity = true);

    /**
     * @brief Sorts the composed EventRegistry in-place using EventSorter and the given Functor.
     * @param comparator Polymorphic EventComparator defining strict weak precedence.
     * @param useQuickSort If true, uses O(n log n) Quicksort; if false, stable O(n^2) Insertion Sort.
     */
    void sortEvents(const EventComparator& comparator, bool useQuickSort = true);

    /**
     * @brief Sorts the composed EventRegistry chronologically via EventSorter::quickSort.
     */
    void sortEventsChronologically();

    /**
     * @brief Clears all channels, pipeline stages, detectors, and events while preserving sessionName.
     */
    void clearAll();

    friend std::ostream& operator<<(std::ostream& os, const StudySession& session);
};

#endif
