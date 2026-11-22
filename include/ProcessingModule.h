
#ifndef PROCESSING_MODULE_H
#define PROCESSING_MODULE_H

#include <string>

/**
* @class ProcessingModule
* @brief Abstract base class for processing module.
* @invariant nameOfModule must be a non-empty standard string.
* @invariant isActive must indicate whether the module is active or inactive.
*/

class ProcessingModule {
    private:
        std::string nameOfModule;
        bool isActive;

    public:
        /**
        * @brief Constructor of the class.
        * @param givenName descriptive name of the module.
        * @param givenState boolean that indicates if the module is active or inactive.
        */
        ProcessingModule(const std::string& givenName, bool givenState);

        /**
        * @brief Virtual destructor: allows you to delete a pointer to the base
        * and execute first the destructor of the derived class and then this one.
        */
        virtual ~ProcessingModule();

        const std::string& getNombre() const;
        bool isActiveModule() const;
        void setActive(bool state);

        /**
        * @brief It receives a sample and returns the filtered sample.
        * If the module is inactive, it must return the unfiltered sample (bypass).
        * @param Sample numerical sample to be processed.
        * @return The filtered sample or the original sample if the module is inactive.
        */
        virtual double processSample(double Sample) = 0;

        /**
        * @brief Clears the internal history so the filter can be reused on another channel.
        */
        virtual void reset() = 0;

        /**
        * @brief Creates a deep copy of the current object, preserving its concrete derived type.
        * @return A pointer to a newly allocated copy of the concrete module.
        */
        virtual ProcessingModule* clone() const = 0;

        /**
        * @brief Serializes the current configuration of the module.
        * @return A string containing the serialized module configuration.
        */
        virtual std::string serializeConfiguration() const = 0;
};

#endif
