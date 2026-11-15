#ifndef DINAMIC_ARR_H
#define DINAMIC_ARR_H

/**
* @class DinamicArr
* @brief Suport class for the project, will take the rol of be a dinamic array for the samples
* @invariant The pointers must be nullptr or point to a valid space of memory on the heap, never daling ptr
* @invariant The number of elements in N must coincide allways with the space in heap
* @invariant This is an array for numbers int and double (native double but can work for int also if needed), also  it needs to be inicialized with valours 
*/
class DinamicArr {
    private:
        int numberOfElements;
        double* nativeArray;
    public:
        /**
        * @brief constructor of the class, it will create the space in the heap in order to keep the samples, will set all the array elements to zero
        * @param ProvidedNumOfElements its the number of elements that the array will have.
        */
        DinamicArr(int ProvidedNumOfElements); 

        /**
        * @brief Creates an array with all the elemnts set, it relies in setElementsInRow method
        * @param rowOfElements Its the pointer to the block of memory to copy
        * @param numberOfElements Its the number of elements to create the space in heap for the array
        */
        DinamicArr(double* rowOfElements, int numberOfElements);

        /**
        * @brief Constructor copy of the class, It will create a DinamicArr reliing on the deep copy of the class to create a copy
        * @param other A selfClass that will be the one copied to the array created
        */
        DinamicArr(const DinamicArr &other);
        
        /**
        * @brief Constructor by default of the class, creates a nullptr array, set the number of elements to zero
        */
        DinamicArr();
        
        /**
        * @brief Destructor of the class it will liberate the memory to avoid memory leaks
        */
        ~DinamicArr();

        /**
        * @brief Sets an element in the position of index provided
        * @param elementProvided Its double that is provided to us for to assing in the index
        * @param indexProvided Its the index that is provided to assing the valour 
        * @return A bool that indicates if the operation was succesful or not
        */
        bool setElement (double elementProvided, int indexProvided);

        /**
        * @brief Set all the array to a block of memory that is provided and copy its elements to the array 
        * @param rowOfElements Its the pointer to the block of memory to copy
        * @param numberOfElements Its the number of elements to create the space in heap for the array
        * @return A bool that indicates if the operation was succesful or not
        */
        bool setElementsInRow ( double* rowOfElements, int numberOfElements);

        int getNumberOfElements();

        /**
        * @brief Brought the element solicited by valour
        * @param index Its the index of the element that its beeing solicited
        * @return A double that is the element solicited
        */
        double getElement (int index);

        /**
        * @brief creates an array in the heap copy of the one contained by the class
        * @return The pointer that points to the memory in the heap that contains the copy of the array
        */
        double* getAllElements ();

        /**
        * @brief The overcharge of the operator = to make a deep copy of the object
        * @param oher another DinamicArr by reference
        * @return A dinamic Arr that is a deep copy 
        */
        DinamicArr operator=(DinamicArr &other);
};
#endif