#include "EventRegistry.h"

#include <cmath>
#include <stdexcept>

EventRegistry::EventRegistry(int initialCapacity)
    : events(nullptr),
      count(0),
      capacity(0) {
    if (initialCapacity < 0) {
        throw std::invalid_argument("EventRegistry: initialCapacity cannot be negative.");
    }
    if (initialCapacity > 0) {
        events = new DetectedEvent*[initialCapacity];
        for (int i = 0; i < initialCapacity; ++i) {
            *(events + i) = nullptr;
        }
        capacity = initialCapacity;
    }
}

EventRegistry::EventRegistry(const EventRegistry& other)
    : events(nullptr),
      count(0),
      capacity(0) {
    if (other.count > 0) {
        events = new DetectedEvent*[other.count];
        capacity = other.count;
        try {
            for (int i = 0; i < other.count; ++i) {
                *(events + i) = (*(other.events + i))->clone();
                ++count;
            }
        } catch (...) {
            releaseResources();
            throw;
        }
    }
}

EventRegistry& EventRegistry::operator=(const EventRegistry& other) {
    if (this == &other) {
        return *this;
    }

    DetectedEvent** newBuffer = nullptr;
    int newCount = 0;
    int newCapacity = other.count;

    if (newCapacity > 0) {
        newBuffer = new DetectedEvent*[newCapacity];
        try {
            for (int i = 0; i < other.count; ++i) {
                *(newBuffer + i) = (*(other.events + i))->clone();
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
    events = newBuffer;
    count = newCount;
    capacity = newCapacity;
    return *this;
}

EventRegistry::~EventRegistry() {
    releaseResources();
}

void EventRegistry::resize(int newCapacity) {
    if (newCapacity <= 0 || newCapacity < count) {
        throw std::invalid_argument("EventRegistry::resize: invalid target capacity.");
    }

    DetectedEvent** newBuffer = new DetectedEvent*[newCapacity];
    for (int i = 0; i < count; ++i) {
        *(newBuffer + i) = *(events + i);
    }
    for (int i = count; i < newCapacity; ++i) {
        *(newBuffer + i) = nullptr;
    }

    delete[] events;
    events = newBuffer;
    capacity = newCapacity;
}

void EventRegistry::releaseResources() {
    for (int i = 0; i < count; ++i) {
        delete *(events + i);
    }
    delete[] events;
    events = nullptr;
    count = 0;
    capacity = 0;
}

void EventRegistry::sortWith(bool (*precedes)(const DetectedEvent&, const DetectedEvent&)) {
    for (int i = 1; i < count; ++i) {
        DetectedEvent* key = *(events + i);
        int j = i - 1;
        while (j >= 0 && precedes(*key, *(*(events + j)))) {
            *(events + j + 1) = *(events + j);
            --j;
        }
        *(events + j + 1) = key;
    }
}

bool EventRegistry::compareChronologically(const DetectedEvent& a, const DetectedEvent& b) {
    return a < b;
}

bool EventRegistry::compareBySeverity(const DetectedEvent& a, const DetectedEvent& b) {
    if (a.getSeverity() != b.getSeverity()) {
        return a.getSeverity() > b.getSeverity();
    }
    return a < b;
}

void EventRegistry::add(const DetectedEvent& event) {
    DetectedEvent* copy = event.clone();
    adopt(copy);
}

void EventRegistry::adopt(DetectedEvent* event) {
    if (event == nullptr) {
        throw std::invalid_argument("EventRegistry::adopt: cannot adopt a null pointer.");
    }
    for (int i = 0; i < count; ++i) {
        if (*(events + i) == event) {
            throw std::invalid_argument("EventRegistry::adopt: pointer is already owned by this registry.");
        }
    }

    if (count == capacity) {
        int newCap = (capacity == 0) ? 4 : capacity * 2;
        try {
            resize(newCap);
        } catch (...) {
            delete event;
            throw;
        }
    }

    *(events + count) = event;
    ++count;
}

void EventRegistry::clear() {
    for (int i = 0; i < count; ++i) {
        delete *(events + i);
        *(events + i) = nullptr;
    }
    count = 0;
}

void EventRegistry::sortChronologically() {
    sortWith(&EventRegistry::compareChronologically);
}

void EventRegistry::sortBySeverity() {
    sortWith(&EventRegistry::compareBySeverity);
}

int EventRegistry::getCount() const {
    return count;
}

int EventRegistry::getCapacity() const {
    return capacity;
}

bool EventRegistry::isEmpty() const {
    return count == 0;
}

const DetectedEvent& EventRegistry::get(int index) const {
    if (index < 0 || index >= count) {
        throw std::out_of_range("EventRegistry::get: index out of range.");
    }
    return *(*(events + index));
}

int EventRegistry::countByType(const std::string& type) const {
    int matches = 0;
    for (int i = 0; i < count; ++i) {
        if ((*(events + i))->getType() == type) {
            ++matches;
        }
    }
    return matches;
}

EventRegistry EventRegistry::filterByType(const std::string& type) const {
    EventRegistry filtered(0);
    for (int i = 0; i < count; ++i) {
        if ((*(events + i))->getType() == type) {
            filtered.add(*(*(events + i)));
        }
    }
    return filtered;
}

EventRegistry EventRegistry::filterByTimeRange(double startTime, double endTime) const {
    if (!std::isfinite(startTime) || !std::isfinite(endTime) || startTime > endTime) {
        throw std::invalid_argument("EventRegistry::filterByTimeRange: invalid time range.");
    }

    EventRegistry filtered(0);
    for (int i = 0; i < count; ++i) {
        const DetectedEvent* ev = *(events + i);
        if (ev->getEndTimeInSecs() >= startTime && ev->getStartTimeInSecs() <= endTime) {
            filtered.add(*ev);
        }
    }
    return filtered;
}

const DetectedEvent& EventRegistry::operator[](int index) const {
    if (index < 0 || index >= count) {
        throw std::out_of_range("EventRegistry::operator[]: index out of range.");
    }
    return *(*(events + index));
}

DetectedEvent& EventRegistry::operator[](int index) {
    if (index < 0 || index >= count) {
        throw std::out_of_range("EventRegistry::operator[]: index out of range.");
    }
    return *(*(events + index));
}

EventRegistry& EventRegistry::operator+=(const DetectedEvent& event) {
    add(event);
    return *this;
}

EventRegistry& EventRegistry::operator+=(const EventRegistry& other) {
    int toCopy = other.count;
    if (toCopy == 0) {
        return *this;
    }

    int requiredCapacity = count + toCopy;
    if (requiredCapacity > capacity) {
        int newCap = (capacity == 0) ? 4 : capacity;
        while (newCap < requiredCapacity) {
            newCap *= 2;
        }
        resize(newCap);
    }

    int cloned = 0;
    try {
        for (int i = 0; i < toCopy; ++i) {
            *(events + count + i) = (*(other.events + i))->clone();
            ++cloned;
        }
    } catch (...) {
        for (int i = 0; i < cloned; ++i) {
            delete *(events + count + i);
            *(events + count + i) = nullptr;
        }
        throw;
    }

    count += cloned;
    return *this;
}

EventRegistry EventRegistry::operator+(const EventRegistry& other) const {
    EventRegistry combined(count + other.count);
    combined += *this;
    combined += other;
    return combined;
}

bool EventRegistry::operator==(const EventRegistry& other) const {
    if (count != other.count) {
        return false;
    }
    for (int i = 0; i < count; ++i) {
        if (*(*(events + i)) != *(*(other.events + i))) {
            return false;
        }
    }
    return true;
}

bool EventRegistry::operator!=(const EventRegistry& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const EventRegistry& registry) {
    os << "EventRegistry (count=" << registry.count << ")\n";
    for (int i = 0; i < registry.count; ++i) {
        os << "  [" << i << "] " << *(*(registry.events + i)) << "\n";
    }
    return os;
}