#include "StudySession.h"

#include "EventComparator.h"
#include "EventSorter.h"
#include "SeverityComparator.h"

#include <cctype>
#include <ostream>
#include <stdexcept>

std::string StudySession::sanitizeSessionName(const std::string& givenName) {
    std::string cleaned;
    cleaned.reserve(givenName.size());
    bool hasNonSpace = false;

    for (std::size_t i = 0; i < givenName.size(); ++i) {
        const char ch = givenName[i];
        if (ch == '\n' || ch == '\r' || ch == ';') {
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            hasNonSpace = true;
        }
        cleaned.push_back(ch);
    }

    return hasNonSpace ? cleaned : "UntitledSession";
}

StudySession::StudySession(const std::string& givenName, int initialChannelCapacity)
    : sessionName(sanitizeSessionName(givenName)),
      channels(nullptr),
      channelCount(0),
      channelCapacity(0),
      pipeline(),
      engine(),
      registry(0) {
    if (initialChannelCapacity < 0) {
        throw std::invalid_argument("StudySession: initialChannelCapacity cannot be negative.");
    }
    if (initialChannelCapacity > 0) {
        channels = new ChannelSignal*[initialChannelCapacity]();
        channelCapacity = initialChannelCapacity;
    }
}

StudySession::StudySession(const StudySession& other)
    : sessionName(other.sessionName),
      channels(nullptr),
      channelCount(0),
      channelCapacity(0),
      pipeline(other.pipeline),
      engine(other.engine),
      registry(other.registry) {
    if (other.channelCount > 0) {
        channels = new ChannelSignal*[other.channelCount]();
        channelCapacity = other.channelCount;
        try {
            for (int i = 0; i < other.channelCount; ++i) {
                *(channels + i) = new ChannelSignal(*(*(other.channels + i)));
                ++channelCount;
            }
        } catch (...) {
            releaseChannels();
            throw;
        }
    }
}

StudySession& StudySession::operator=(const StudySession& other) {
    if (this == &other) {
        return *this;
    }

    ChannelSignal** newChannels = nullptr;
    int newCount = 0;
    const int newCapacity = other.channelCount;

    if (newCapacity > 0) {
        newChannels = new ChannelSignal*[newCapacity]();
        try {
            for (int i = 0; i < other.channelCount; ++i) {
                *(newChannels + i) = new ChannelSignal(*(*(other.channels + i)));
                ++newCount;
            }
        } catch (...) {
            for (int i = 0; i < newCount; ++i) {
                delete *(newChannels + i);
            }
            delete[] newChannels;
            throw;
        }
    }

    try {
        ProcessingPipeline newPipeline(other.pipeline);
        DetectionEngine newEngine(other.engine);
        EventRegistry newRegistry(other.registry);
        std::string newName(other.sessionName);

        pipeline = newPipeline;
        engine = newEngine;
        registry = newRegistry;
        sessionName = newName;
    } catch (...) {
        for (int i = 0; i < newCount; ++i) {
            delete *(newChannels + i);
        }
        delete[] newChannels;
        throw;
    }

    releaseChannels();
    channels = newChannels;
    channelCount = newCount;
    channelCapacity = newCapacity;
    return *this;
}

StudySession::~StudySession() {
    releaseChannels();
}

void StudySession::resizeChannels(int newCapacity) {
    if (newCapacity <= 0 || newCapacity < channelCount) {
        throw std::invalid_argument("StudySession::resizeChannels: invalid target capacity.");
    }

    ChannelSignal** newBuffer = new ChannelSignal*[newCapacity]();
    for (int i = 0; i < channelCount; ++i) {
        *(newBuffer + i) = *(channels + i);
    }

    delete[] channels;
    channels = newBuffer;
    channelCapacity = newCapacity;
}

void StudySession::releaseChannels() {
    for (int i = 0; i < channelCount; ++i) {
        delete *(channels + i);
    }
    delete[] channels;
    channels = nullptr;
    channelCount = 0;
    channelCapacity = 0;
}

const std::string& StudySession::getSessionName() const {
    return sessionName;
}

void StudySession::setSessionName(const std::string& givenName) {
    sessionName = sanitizeSessionName(givenName);
}

void StudySession::addChannel(const ChannelSignal& signal) {
    ChannelSignal* copy = new ChannelSignal(signal);
    adoptChannel(copy);
}

void StudySession::adoptChannel(ChannelSignal* signal) {
    if (signal == nullptr) {
        throw std::invalid_argument("StudySession::adoptChannel: cannot adopt a null pointer.");
    }
    for (int i = 0; i < channelCount; ++i) {
        if (*(channels + i) == signal) {
            throw std::invalid_argument("StudySession::adoptChannel: channel pointer is already owned.");
        }
    }

    if (channelCount == channelCapacity) {
        const int newCap = (channelCapacity == 0) ? kDefaultChannelCapacity : channelCapacity * 2;
        try {
            resizeChannels(newCap);
        } catch (...) {
            delete signal;
            throw;
        }
    }

    *(channels + channelCount) = signal;
    ++channelCount;
}

bool StudySession::removeChannel(int index) {
    if (index < 0 || index >= channelCount) {
        return false;
    }

    delete *(channels + index);
    for (int i = index; i < channelCount - 1; ++i) {
        *(channels + i) = *(channels + i + 1);
    }
    --channelCount;
    *(channels + channelCount) = nullptr;
    return true;
}

void StudySession::clearChannels() {
    for (int i = 0; i < channelCount; ++i) {
        delete *(channels + i);
        *(channels + i) = nullptr;
    }
    channelCount = 0;
}

int StudySession::getChannelCount() const {
    return channelCount;
}

int StudySession::getChannelCapacity() const {
    return channelCapacity;
}

bool StudySession::hasChannels() const {
    return channelCount > 0;
}

const ChannelSignal& StudySession::getChannel(int index) const {
    if (index < 0 || index >= channelCount) {
        throw std::out_of_range("StudySession::getChannel: index out of range.");
    }
    return *(*(channels + index));
}

ChannelSignal& StudySession::getChannel(int index) {
    if (index < 0 || index >= channelCount) {
        throw std::out_of_range("StudySession::getChannel: index out of range.");
    }
    return *(*(channels + index));
}

int StudySession::findChannelByCode(const std::string& givenCode) const {
    for (int i = 0; i < channelCount; ++i) {
        if ((*(channels + i))->getMetadata().getCodeOfChannel() == givenCode) {
            return i;
        }
    }
    return -1;
}

ProcessingPipeline& StudySession::getPipeline() {
    return pipeline;
}

const ProcessingPipeline& StudySession::getPipeline() const {
    return pipeline;
}

DetectionEngine& StudySession::getEngine() {
    return engine;
}

const DetectionEngine& StudySession::getEngine() const {
    return engine;
}

EventRegistry& StudySession::getRegistry() {
    return registry;
}

const EventRegistry& StudySession::getRegistry() const {
    return registry;
}

bool StudySession::applyPipelineToChannel(int index, const std::string& sourceBuffer) {
    if (index < 0 || index >= channelCount) {
        return false;
    }
    return (*(channels + index))->applyPipeline(pipeline, sourceBuffer);
}

bool StudySession::applyPipelineToAll(const std::string& sourceBuffer) {
    if (sourceBuffer != "raw" && sourceBuffer != "processed") {
        return false;
    }
    for (int i = 0; i < channelCount; ++i) {
        (*(channels + i))->applyPipeline(pipeline, sourceBuffer);
    }
    return true;
}

void StudySession::restoreAllChannels() {
    for (int i = 0; i < channelCount; ++i) {
        (*(channels + i))->restoreProcessedSamples();
    }
}

int StudySession::runAnalysis(bool sortBySeverity) {
    registry.clear();
    if (channelCount == 0 || engine.isEmpty()) {
        return 0;
    }

    const ChannelSignal** constView = new const ChannelSignal*[channelCount];
    for (int i = 0; i < channelCount; ++i) {
        *(constView + i) = *(channels + i);
    }

    try {
        registry = engine.runMultiple(constView, channelCount, false);
    } catch (...) {
        delete[] constView;
        throw;
    }
    delete[] constView;

    if (sortBySeverity) {
        const SeverityComparator bySevDesc(true);
        EventSorter::quickSort(registry, bySevDesc);
    } else {
        EventSorter::quickSort(registry);
    }

    return registry.getCount();
}

void StudySession::sortEvents(const EventComparator& comparator, bool useQuickSort) {
    if (useQuickSort) {
        EventSorter::quickSort(registry, comparator);
    } else {
        EventSorter::insertionSort(registry, comparator);
    }
}

void StudySession::sortEventsChronologically() {
    EventSorter::quickSort(registry);
}

void StudySession::clearAll() {
    clearChannels();
    pipeline.clear();
    engine.clearDetectors();
    registry.clear();
}

std::ostream& operator<<(std::ostream& os, const StudySession& session) {
    os << "StudySession \"" << session.sessionName << "\" "
       << "(channels=" << session.channelCount
       << ", pipelineStages=" << session.pipeline.getCount()
       << ", detectors=" << session.engine.getDetectorCount()
       << ", events=" << session.registry.getCount() << ")";
    return os;
}
