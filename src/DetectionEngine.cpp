#include "DetectionEngine.h"

#include <stdexcept>

DetectionEngine::DetectionEngine(int initialCapacity)
    : detectors(nullptr),
      count(0),
      capacity(0) {
    if (initialCapacity < 0) {
        throw std::invalid_argument("DetectionEngine: initialCapacity cannot be negative.");
    }
    if (initialCapacity > 0) {
        detectors = new EventDetector*[initialCapacity];
        for (int i = 0; i < initialCapacity; ++i) {
            *(detectors + i) = nullptr;
        }
        capacity = initialCapacity;
    }
}

DetectionEngine::DetectionEngine(const DetectionEngine& other)
    : detectors(nullptr),
      count(0),
      capacity(0) {
    if (other.count > 0) {
        detectors = new EventDetector*[other.count];
        capacity = other.count;
        try {
            for (int i = 0; i < other.count; ++i) {
                *(detectors + i) = (*(other.detectors + i))->clone();
                ++count;
            }
        } catch (...) {
            releaseResources();
            throw;
        }
    }
}

DetectionEngine& DetectionEngine::operator=(const DetectionEngine& other) {
    if (this == &other) {
        return *this;
    }

    EventDetector** newBuffer = nullptr;
    int newCount = 0;
    int newCapacity = other.count;

    if (newCapacity > 0) {
        newBuffer = new EventDetector*[newCapacity];
        try {
            for (int i = 0; i < other.count; ++i) {
                *(newBuffer + i) = (*(other.detectors + i))->clone();
                ++newCount;
            }
        } catch (...) {
            for (int i = 0; i < newCount; ++i) {
                delete *(newBuffer + i);
            }
            delete[] newBuffer;
            throw;
        }
    }

    releaseResources();
    detectors = newBuffer;
    count = newCount;
    capacity = newCapacity;
    return *this;
}

DetectionEngine::~DetectionEngine() {
    releaseResources();
}

void DetectionEngine::resize(int newCapacity) {
    if (newCapacity <= 0 || newCapacity < count) {
        throw std::invalid_argument("DetectionEngine::resize: invalid target capacity.");
    }

    EventDetector** newBuffer = new EventDetector*[newCapacity];
    for (int i = 0; i < count; ++i) {
        *(newBuffer + i) = *(detectors + i);
    }
    for (int i = count; i < newCapacity; ++i) {
        *(newBuffer + i) = nullptr;
    }

    delete[] detectors;
    detectors = newBuffer;
    capacity = newCapacity;
}

void DetectionEngine::releaseResources() {
    for (int i = 0; i < count; ++i) {
        delete *(detectors + i);
    }
    delete[] detectors;
    detectors = nullptr;
    count = 0;
    capacity = 0;
}

void DetectionEngine::addDetector(const EventDetector& detector) {
    EventDetector* copy = detector.clone();
    adoptDetector(copy);
}

void DetectionEngine::adoptDetector(EventDetector* detector) {
    if (detector == nullptr) {
        throw std::invalid_argument("DetectionEngine::adoptDetector: cannot adopt a null pointer.");
    }
    for (int i = 0; i < count; ++i) {
        if (*(detectors + i) == detector) {
            throw std::invalid_argument("DetectionEngine::adoptDetector: pointer is already owned by this engine.");
        }
    }

    if (count == capacity) {
        const int newCap = (capacity == 0) ? 4 : capacity * 2;
        try {
            resize(newCap);
        } catch (...) {
            delete detector;
            throw;
        }
    }

    *(detectors + count) = detector;
    ++count;
}

void DetectionEngine::clearDetectors() {
    for (int i = 0; i < count; ++i) {
        delete *(detectors + i);
        *(detectors + i) = nullptr;
    }
    count = 0;
}

int DetectionEngine::getDetectorCount() const {
    return count;
}

int DetectionEngine::getCapacity() const {
    return capacity;
}

bool DetectionEngine::isEmpty() const {
    return count == 0;
}

const EventDetector& DetectionEngine::getDetector(int index) const {
    if (index < 0 || index >= count) {
        throw std::out_of_range("DetectionEngine::getDetector: index out of range.");
    }
    return *(*(detectors + index));
}

EventRegistry DetectionEngine::run(const ChannelSignal& signal, bool sortBySeverityOrder) const {
    EventRegistry combined(0);
    for (int i = 0; i < count; ++i) {
        combined += (*(detectors + i))->detect(signal);
    }
    if (sortBySeverityOrder) {
        combined.sortBySeverity();
    } else {
        combined.sortChronologically();
    }
    return combined;
}

EventRegistry DetectionEngine::runMultiple(const ChannelSignal* const* signals,
                                           int signalCount,
                                           bool sortBySeverityOrder) const {
    if (signalCount < 0) {
        throw std::invalid_argument("DetectionEngine::runMultiple: signalCount cannot be negative.");
    }
    if (signalCount > 0 && signals == nullptr) {
        throw std::invalid_argument("DetectionEngine::runMultiple: signals array cannot be null.");
    }
    for (int s = 0; s < signalCount; ++s) {
        if (*(signals + s) == nullptr) {
            throw std::invalid_argument("DetectionEngine::runMultiple: signal entry cannot be null.");
        }
    }

    EventRegistry combined(0);
    for (int s = 0; s < signalCount; ++s) {
        const ChannelSignal& current = *(*(signals + s));
        for (int i = 0; i < count; ++i) {
            combined += (*(detectors + i))->detect(current);
        }
    }
    if (sortBySeverityOrder) {
        combined.sortBySeverity();
    } else {
        combined.sortChronologically();
    }
    return combined;
}

std::ostream& operator<<(std::ostream& os, const DetectionEngine& engine) {
    os << "DetectionEngine (detectors=" << engine.count << ")\n";
    for (int i = 0; i < engine.count; ++i) {
        os << "  [" << i << "] " << *(*(engine.detectors + i)) << "\n";
    }
    return os;
}