#include "ProcessingModule.h"

ProcessingModule::ProcessingModule(const std::string& givenName)
    : name(givenName.empty() ? "Unnamed" : givenName), active(true) {}

ProcessingModule::~ProcessingModule() {}

const std::string& ProcessingModule::getName() const {return name;}
bool ProcessingModule::isActive() const {return active;}
void ProcessingModule::setActive(bool givenState) {active = givenState;}
