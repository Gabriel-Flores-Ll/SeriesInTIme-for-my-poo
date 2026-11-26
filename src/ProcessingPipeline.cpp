#include "ProcessingPipeline.h"

#include "ProcessingModule.h"

ProcessingPipeline::ProcessingPipeline()
    : stages(new ProcessingModule*[INITIAL_CAPACITY]()), count(0), capacity(INITIAL_CAPACITY) {}

ProcessingPipeline::ProcessingPipeline(const ProcessingPipeline& other)
    : stages(new ProcessingModule*[other.capacity]()), count(other.count), capacity(other.capacity) {
    for (int i = 0; i < count; i++) {
        *(stages + i) = (*(other.stages + i))->clone();    // each stage is cloned according to its real type
    }
}

ProcessingPipeline& ProcessingPipeline::operator=(const ProcessingPipeline& other) {
    if (this != &other) {
        ProcessingModule** newStages = new ProcessingModule*[other.capacity]();
        for (int i = 0; i < other.count; i++) {
            *(newStages + i) = (*(other.stages + i))->clone();
        }
        freeStages();                                      // only now the old stages are destroyed
        stages = newStages;
        count = other.count;
        capacity = other.capacity;
    }
    return *this;
}

ProcessingPipeline::~ProcessingPipeline() {freeStages();}

void ProcessingPipeline::freeStages() {
    for (int i = 0; i < count; i++) {
        delete *(stages + i);          // virtual destructor: the real leaf class is destroyed
    }
    delete[] stages;
    stages = nullptr;
    count = 0;
}

void ProcessingPipeline::doubleCapacity() {
    int newCapacity = capacity * 2;
    ProcessingModule** newStages = new ProcessingModule*[newCapacity]();
    for (int i = 0; i < count; i++) {
        *(newStages + i) = *(stages + i);  // only the pointers are moved, the objects are not touched
    }
    delete[] stages;
    stages = newStages;
    capacity = newCapacity;
}

bool ProcessingPipeline::addStage(ProcessingModule* givenModule) {
    if (givenModule == nullptr) {return false;}
    if (count == capacity) {doubleCapacity();}
    *(stages + count) = givenModule;
    count++;
    return true;
}

double ProcessingPipeline::processSample(double givenSample) {
    for (int i = 0; i < count; i++) {
        givenSample = (*(stages + i))->processSample(givenSample);   // dynamic dispatch
    }
    return givenSample;
}

void ProcessingPipeline::reset() {
    for (int i = 0; i < count; i++) {(*(stages + i))->reset();}
}

bool ProcessingPipeline::setStageActive(int givenIndex, bool givenState) {
    if (givenIndex < 0 || givenIndex >= count) {return false;}
    (*(stages + givenIndex))->setActive(givenState);
    return true;
}

bool ProcessingPipeline::removeStage(int givenIndex) {
    if (givenIndex < 0 || givenIndex >= count) {return false;}
    delete *(stages + givenIndex);
    for (int i = givenIndex; i < count - 1; i++) {
        *(stages + i) = *(stages + i + 1);                 // shifts the stages one position to the left
    }
    count--;
    *(stages + count) = nullptr;
    return true;
}

int ProcessingPipeline::getCount() const {return count;}
int ProcessingPipeline::getCapacity() const {return capacity;}

std::string ProcessingPipeline::serialize() const {
    std::string result;
    for (int i = 0; i < count; i++) {
        if (i > 0) {result += " | ";}
        result += (*(stages + i))->serialize();
    }
    return result;
}
