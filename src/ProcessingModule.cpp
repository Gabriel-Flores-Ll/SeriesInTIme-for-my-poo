#include "ProcessingModule.h"

#include <cctype>
#include <ostream>

std::string ProcessingModule::sanitizeName(const std::string& givenName) {
    std::string cleaned;
    cleaned.reserve(givenName.size());
    bool hasNonSpace = false;

    for (std::size_t i = 0; i < givenName.size(); ++i) {
        const char ch = givenName[i];
        if (ch == ';' || ch == '|' || ch == '\n' || ch == '\r') {
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            hasNonSpace = true;
        }
        cleaned.push_back(ch);
    }

    return hasNonSpace ? cleaned : "Unnamed";
}

ProcessingModule::ProcessingModule(const std::string& givenName)
    : name(sanitizeName(givenName)),
      active(true) {}

const std::string& ProcessingModule::getName() const {
    return name;
}

bool ProcessingModule::isActive() const {
    return active;
}

void ProcessingModule::setActive(bool givenState) {
    active = givenState;
}

std::ostream& operator<<(std::ostream& os, const ProcessingModule& module) {
    os << module.serialize();
    return os;
}
