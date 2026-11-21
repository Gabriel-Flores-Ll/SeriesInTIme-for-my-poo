#ifndef DINAMIC_ARR_H
#define DINAMIC_ARR_H

/**
 * @class DinamicArr
 * @brief RAII wrapper managing a heap-allocated contiguous array of doubles without STL containers.
 * @invariant numberOfElements >= 0.
 * @invariant If numberOfElements == 0, nativeArray == nullptr.
 * @invariant If numberOfElements > 0, nativeArray points to a valid heap block of size numberOfElements.
 */
class DinamicArr {
private:
    int numberOfElements;
    double* nativeArray;

public:
    /**
     * @brief Default constructor creating an empty array (nullptr, size 0).
     */
    DinamicArr();

    /**
     * @brief Allocates a zero-initialized array of the requested size.
     * @param providedNumOfElements Number of elements (> 0; non-positive values create an empty array).
     */
    explicit DinamicArr(int providedNumOfElements);

    /**
     * @brief Allocates and deep-copies elements from a source memory block.
     * @param rowOfElements Pointer to source data (must be non-null for copy to occur).
     * @param providedNumberOfElements Number of elements to copy (> 0).
     */
    DinamicArr(const double* rowOfElements, int providedNumberOfElements);

    /**
     * @brief Deep copy constructor (Rule of Three).
     * @param other Source DinamicArr to copy.
     */
    DinamicArr(const DinamicArr& other);

    /**
     * @brief Destructor releasing heap memory (Rule of Three).
     */
    ~DinamicArr();

    /**
     * @brief Deep copy assignment operator with self-assignment guard and strong safety (Rule of Three).
     * @param other Source DinamicArr to copy.
     * @return Reference to *this.
     */
    DinamicArr& operator=(const DinamicArr& other);

    /**
     * @brief Assigns a value at the given index if within bounds [0, numberOfElements).
     * @param elementProvided Value to write.
     * @param indexProvided Target position.
     * @return true if index was valid and updated, false otherwise.
     */
    bool setElement(double elementProvided, int indexProvided);

    /**
     * @brief Replaces the entire internal buffer with a deep copy of the provided block.
     * @param rowOfElements Non-null pointer to source elements.
     * @param providedNumberOfElements Number of elements (> 0).
     * @return true if replaced successfully, false if arguments were invalid (previous state preserved).
     */
    bool setElementsInRow(const double* rowOfElements, int providedNumberOfElements);

    int getNumberOfElements() const;

    /**
     * @brief Retrieves the element at index; returns NaN if index is out of bounds.
     * @param index Position in [0, numberOfElements).
     * @return Sample value at index, or quiet_NaN() if invalid.
     */
    double getElement(int index) const;

    /**
     * @brief Provides read-only access to the underlying contiguous buffer for internal consumers (zero-copy).
     * @return Const pointer to the first element, or nullptr if the array is empty.
     */
    const double* getData() const;
};

#endif
