#include "ProcessingModule.h"

ProcessingModule::ProcessingModule(const std::string& givenName, bool givenState)
    : nameOfModule(givenName.empty() ? "SinNombre" : givenName),
      isActive(givenState) {}

ProcessingModule::~ProcessingModule() {}

const std::string& ProcessingModule::getNombre() const {return nameOfModule;}

bool ProcessingModule::isActiveModule() const {return isActive;}

void ProcessingModule::setActive(bool state) {isActive = state;}
