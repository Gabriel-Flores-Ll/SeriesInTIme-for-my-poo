#include "ProcessingPipeline.h"

#include "ProcessingModule.h"

#include <ostream>
#include <stdexcept>

ProcessingPipeline::ProcessingPipeline(int initialCapacity)
    : stages(nullptr),
      count(0),
      capacity(0) {
    if (initialCapacity < 0) {
        throw std::invalid_argument("ProcessingPipeline: initialCapacity cannot be negative.");
    }
    if (initialCapacity > 0) {
        stages = new ProcessingModule*[initialCapacity]();
        capacity = initialCapacity;
    }
}

ProcessingPipeline::ProcessingPipeline(const ProcessingPipeline& other)
    : stages(nullptr),
      count(0),
      capacity(0) {
    if (other.count > 0) {
        stages = new ProcessingModule*[other.count]();
        capacity = other.count;
        try {
            for (int i = 0; i < other.count; ++i) {
                *(stages + i) = (*(other.stages + i))->clone();
                ++count;
            }
        } catch (...) {
            freeStages();
            throw;
        }
    }
}

ProcessingPipeline& ProcessingPipeline::operator=(const ProcessingPipeline& other) {
    if (this == &other) {
        return *this;
    }

    ProcessingModule** newStages = nullptr;
    int newCount = 0;
    const int newCapacity = other.count;

    if (newCapacity > 0) {
        newStages = new ProcessingModule*[newCapacity]();
        try {
            for (int i = 0; i < other.count; ++i) {
                *(newStages + i) = (*(other.stages + i))->clone();
                ++newCount;
            }
        } catch (...) {
            for (int i = 0; i < newCount; ++i) {
                delete *(newStages + i);
            }
            delete[] newStages;
            throw;
        }
    }

    freeStages();
    stages = newStages;
    count = newCount;
    capacity = newCapacity;
    return *this;
}

ProcessingPipeline::~ProcessingPipeline() {
    freeStages();
}

void ProcessingPipeline::resize(int newCapacity) {
    if (newCapacity <= 0 || newCapacity < count) {
        throw std::invalid_argument("ProcessingPipeline::resize: invalid target capacity.");
    }

    ProcessingModule** newStages = new ProcessingModule*[newCapacity]();
    for (int i = 0; i < count; ++i) {
        *(newStages + i) = *(stages + i);
    }

    delete[] stages;
    stages = newStages;
    capacity = newCapacity;
}

void ProcessingPipeline::freeStages() {
    for (int i = 0; i < count; ++i) {
        delete *(stages + i);
    }
    delete[] stages;
    stages = nullptr;
    count = 0;
    capacity = 0;
}

void ProcessingPipeline::addStage(const ProcessingModule& givenModule) {
    ProcessingModule* copy = givenModule.clone();
    adoptStage(copy);
}

void ProcessingPipeline::adoptStage(ProcessingModule* givenModule) {
    if (givenModule == nullptr) {
        throw std::invalid_argument("ProcessingPipeline::adoptStage: cannot adopt a null pointer.");
    }
    for (int i = 0; i < count; ++i) {
        if (*(stages + i) == givenModule) {
            throw std::invalid_argument("ProcessingPipeline::adoptStage: module is already owned by this pipeline.");
        }
    }

    if (count == capacity) {
        const int newCap = (capacity == 0) ? kDefaultCapacity : capacity * 2;
        try {
            resize(newCap);
        } catch (...) {
            delete givenModule;
            throw;
        }
    }

    *(stages + count) = givenModule;
    ++count;
}

bool ProcessingPipeline::removeStage(int givenIndex) {
    if (givenIndex < 0 || givenIndex >= count) {
        return false;
    }

    delete *(stages + givenIndex);
    for (int i = givenIndex; i < count - 1; ++i) {
        *(stages + i) = *(stages + i + 1);
    }
    --count;
    *(stages + count) = nullptr;

    reset();
    return true;
}

void ProcessingPipeline::clear() {
    for (int i = 0; i < count; ++i) {
        delete *(stages + i);
        *(stages + i) = nullptr;
    }
    count = 0;
}

bool ProcessingPipeline::setStageActive(int givenIndex, bool givenState) {
    if (givenIndex < 0 || givenIndex >= count) {
        return false;
    }
    (*(stages + givenIndex))->setActive(givenState);
    return true;
}

double ProcessingPipeline::processSample(double givenSample) {
    for (int i = 0; i < count; ++i) {
        givenSample = (*(stages + i))->processSample(givenSample);
    }
    return givenSample;
}

void ProcessingPipeline::reset() {
    for (int i = 0; i < count; ++i) {
        (*(stages + i))->reset();
    }
}

int ProcessingPipeline::getCount() const {
    return count;
}

int ProcessingPipeline::getCapacity() const {
    return capacity;
}

bool ProcessingPipeline::isEmpty() const {
    return count == 0;
}

const ProcessingModule& ProcessingPipeline::getStage(int givenIndex) const {
    if (givenIndex < 0 || givenIndex >= count) {
        throw std::out_of_range("ProcessingPipeline::getStage: index out of range.");
    }
    return *(*(stages + givenIndex));
}

const ProcessingModule& ProcessingPipeline::operator[](int givenIndex) const {
    return getStage(givenIndex);
}

std::string ProcessingPipeline::serialize() const {
    std::string result;
    for (int i = 0; i < count; ++i) {
        if (i > 0) {
            result += " | ";
        }
        result += (*(stages + i))->serialize();
    }
    return result;
}

bool ProcessingPipeline::operator==(const ProcessingPipeline& other) const {
    if (count != other.count) {
        return false;
    }
    for (int i = 0; i < count; ++i) {
        if ((*(stages + i))->serialize() != (*(other.stages + i))->serialize()) {
            return false;
        }
    }
    return true;
}

bool ProcessingPipeline::operator!=(const ProcessingPipeline& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const ProcessingPipeline& pipeline) {
    os << "ProcessingPipeline (stages=" << pipeline.count << ")\n";
    for (int i = 0; i < pipeline.count; ++i) {
        os << "  [" << i << "] " << *(*(pipeline.stages + i)) << "\n";
    }
    return os;
}
