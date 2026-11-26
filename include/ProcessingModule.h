#ifndef PROCESSING_MODULE_H
#define PROCESSING_MODULE_H

#include <string>

/**
* @class ProcessingModule
* @brief Abstract base class (Level 0) of the whole processing hierarchy. It defines the contract that every
*        module must fulfil: process a sample, reset itself, clone itself and serialize its configuration.
* @invariant name != ""
*/
class ProcessingModule {
    private:
        std::string name;
        bool active;
    public:
        /**
        * @brief constructor of the base class, the module starts active
        * @param givenName descriptive name of the module, if it is empty it is corrected to "Unnamed"
        */
        explicit ProcessingModule(const std::string& givenName);

        /**
        * @brief VIRTUAL destructor, it makes the delete over a base pointer to run first the destructor of the concrete
        *        class (freeing its own buffers) and then this one
        */
        virtual ~ProcessingModule();

        /**
        * @brief gives the name of the module
        * @return the descriptive name of the module, it is never empty
        */
        const std::string& getName() const;

        /**
        * @brief indicates if the module is processing or if it lets the signal pass untouched
        * @return true if the module is active, false if it is in bypass
        */
        bool isActive() const;

        /**
        * @brief activates or deactivates the module
        * @param givenState true to activate it, false to put it in bypass
        */
        void setActive(bool givenState);

        /**
        * @brief receives a sample and gives back the filtered one, if the module is inactive the sample must go through intact (bypass)
        * @param givenSample the sample to be processed
        * @return the processed sample, or the same given sample if the module is inactive
        */
        virtual double processSample(double givenSample) = 0;

        /**
        * @brief clears the internal history in order to reuse the module over another signal without residues
        */
        virtual void reset() = 0;

        /**
        * @brief Prototype pattern: creates a deep copy of the module of the SAME concrete type
        * @return a pointer to a new object in the heap, the caller is responsible of doing delete over it
        */
        virtual ProcessingModule* clone() const = 0;

        /**
        * @brief builds a text with a unique label and the current parameters, for example "MOVING_AVERAGE;W=5"
        * @return the text with the configuration of the module
        */
        virtual std::string serialize() const = 0;
};

#endif
