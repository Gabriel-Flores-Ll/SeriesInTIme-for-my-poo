#ifndef PROCESSING_PIPELINE_H
#define PROCESSING_PIPELINE_H

#include <string>

class ProcessingModule;   // forward declaration: the header does not need the complete class

/**
* @class ProcessingPipeline
* @brief Heterogeneous collection of chained modules: the output of stage i is the input of stage i + 1.
*        It stores pointers to the base class, so each stage runs its own version by dynamic dispatch.
*        The pipeline OWNS the modules it receives (it frees them in its destructor).
* @invariant 0 <= count <= capacity
* @invariant stages[0 .. count - 1] are valid (not null) pointers to objects in the heap
*/
class ProcessingPipeline {
    private:
        static const int INITIAL_CAPACITY = 4;

        ProcessingModule** stages;   // dynamic array of pointers to the base class
        int count;
        int capacity;

        /**
        * @brief doubles the capacity: reserves a new array, moves the pointers and frees the old array
        */
        void doubleCapacity();

        /**
        * @brief makes delete over each module (polymorphic) and then over the container array
        */
        void freeStages();
    public:
        /**
        * @brief constructor of an empty pipeline with the initial capacity of 4 stages
        */
        ProcessingPipeline();

        /**
        * @brief DEEP copy constructor (Rule of Three), it clones every stage with clone()
        * @param other the pipeline to be copied
        */
        ProcessingPipeline(const ProcessingPipeline& other);

        /**
        * @brief assignment operator (Rule of Three), protected against self-assignment, it clones every stage
        * @param other the pipeline to be copied
        * @return a reference to this object
        */
        ProcessingPipeline& operator=(const ProcessingPipeline& other);

        /**
        * @brief destructor, it frees every stage polymorphically and then the container array
        */
        ~ProcessingPipeline();

        /**
        * @brief adds a stage at the end of the pipeline, the pipeline takes the ownership of the pointer
        * @param givenModule an object created with new
        * @return true if the stage was added, false if givenModule is nullptr (nothing is added)
        */
        bool addStage(ProcessingModule* givenModule);

        /**
        * @brief passes the sample through all the stages in order (inactive stages let it go through intact)
        * @param givenSample the sample to be processed
        * @return the sample after the last stage, or the same sample if the pipeline is empty
        */
        double processSample(double givenSample);

        /**
        * @brief resets the history of all the stages
        */
        void reset();

        /**
        * @brief activates or deactivates the stage at a given position
        * @param givenIndex position of the stage, it must be in [0, count)
        * @param givenState true to activate it, false to deactivate it
        * @return true if the state was changed, false if the index is out of range
        */
        bool setStageActive(int givenIndex, bool givenState);

        /**
        * @brief removes (and frees) the stage at a given position, the following stages are shifted to the left
        * @param givenIndex position of the stage, it must be in [0, count)
        * @return true if the stage was removed, false if the index is out of range
        */
        bool removeStage(int givenIndex);

        /**
        * @brief gives the current number of stages
        * @return the quantity of stages stored
        */
        int getCount() const;

        /**
        * @brief gives the current capacity of the internal array
        * @return the maximum number of stages that fit before the next resize
        */
        int getCapacity() const;

        /**
        * @brief builds a text with the configuration of all the stages
        * @return the labels of every stage separated by " | ", or an empty text if there are no stages
        */
        std::string serialize() const;
};

#endif
