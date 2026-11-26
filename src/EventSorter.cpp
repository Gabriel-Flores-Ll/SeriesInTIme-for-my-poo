#include "EventSorter.h"

#include "DetectecEvent.h"
#include "EventComparator.h"
#include "EventRegistry.h"

void EventSorter::swapPointers(DetectedEvent** first, DetectedEvent** second) {
    if (first != second) {
        DetectedEvent* temp = *first;
        *first = *second;
        *second = temp;
    }
}

int EventSorter::medianOfThreeRelational(DetectedEvent** arr, int low, int high) {
    const int mid = low + (high - low) / 2;

    if (*(*(arr + mid)) < *(*(arr + low))) {
        swapPointers(arr + low, arr + mid);
    }
    if (*(*(arr + high)) < *(*(arr + low))) {
        swapPointers(arr + low, arr + high);
    }
    if (*(*(arr + high)) < *(*(arr + mid))) {
        swapPointers(arr + mid, arr + high);
    }
    return mid;
}

void EventSorter::quickSortRelationalRange(DetectedEvent** arr, int low, int high) {
    while (low < high) {
        // 3-way (Dutch National Flag) partition around median-of-three pivot prevents O(n^2)
        // degradation on already-sorted, reverse-sorted, or duplicate-heavy event registries.
        const int pivotIdx = medianOfThreeRelational(arr, low, high);
        swapPointers(arr + pivotIdx, arr + low);
        const DetectedEvent* pivot = *(arr + low);

        int lt = low;
        int i = low + 1;
        int gt = high;

        while (i <= gt) {
            if (*(*(arr + i)) < *pivot) {
                swapPointers(arr + lt, arr + i);
                ++lt;
                ++i;
            } else if (*pivot < *(*(arr + i))) {
                swapPointers(arr + i, arr + gt);
                --gt;
            } else {
                ++i;
            }
        }

        // Recurse on the smaller subrange and iterate on the larger one -> O(log n) stack space.
        if (lt - low < high - gt) {
            if (low < lt - 1) {
                quickSortRelationalRange(arr, low, lt - 1);
            }
            low = gt + 1;
        } else {
            if (gt + 1 < high) {
                quickSortRelationalRange(arr, gt + 1, high);
            }
            high = lt - 1;
        }
    }
}

int EventSorter::medianOfThreeComparator(DetectedEvent** arr,
                                         int low,
                                         int high,
                                         const EventComparator& comparator) {
    const int mid = low + (high - low) / 2;

    if (comparator(*(*(arr + mid)), *(*(arr + low)))) {
        swapPointers(arr + low, arr + mid);
    }
    if (comparator(*(*(arr + high)), *(*(arr + low)))) {
        swapPointers(arr + low, arr + high);
    }
    if (comparator(*(*(arr + high)), *(*(arr + mid)))) {
        swapPointers(arr + mid, arr + high);
    }
    return mid;
}

void EventSorter::quickSortComparatorRange(DetectedEvent** arr,
                                           int low,
                                           int high,
                                           const EventComparator& comparator) {
    while (low < high) {
        const int pivotIdx = medianOfThreeComparator(arr, low, high, comparator);
        swapPointers(arr + pivotIdx, arr + low);
        const DetectedEvent* pivot = *(arr + low);

        int lt = low;
        int i = low + 1;
        int gt = high;

        while (i <= gt) {
            if (comparator(*(*(arr + i)), *pivot)) {
                swapPointers(arr + lt, arr + i);
                ++lt;
                ++i;
            } else if (comparator(*pivot, *(*(arr + i)))) {
                swapPointers(arr + i, arr + gt);
                --gt;
            } else {
                ++i;
            }
        }

        if (lt - low < high - gt) {
            if (low < lt - 1) {
                quickSortComparatorRange(arr, low, lt - 1, comparator);
            }
            low = gt + 1;
        } else {
            if (gt + 1 < high) {
                quickSortComparatorRange(arr, gt + 1, high, comparator);
            }
            high = lt - 1;
        }
    }
}

void EventSorter::quickSort(EventRegistry& registry) {
    if (registry.count <= 1) {
        return;
    }
    quickSortRelationalRange(registry.events, 0, registry.count - 1);
}

void EventSorter::quickSort(EventRegistry& registry, const EventComparator& comparator) {
    if (registry.count <= 1) {
        return;
    }
    quickSortComparatorRange(registry.events, 0, registry.count - 1, comparator);
}

void EventSorter::insertionSort(EventRegistry& registry, const EventComparator& comparator) {
    if (registry.count <= 1) {
        return;
    }

    DetectedEvent** arr = registry.events;
    for (int i = 1; i < registry.count; ++i) {
        DetectedEvent* key = *(arr + i);
        int j = i - 1;
        while (j >= 0 && comparator(*key, *(*(arr + j)))) {
            *(arr + j + 1) = *(arr + j);
            --j;
        }
        *(arr + j + 1) = key;
    }
}

bool EventSorter::isSorted(const EventRegistry& registry) {
    if (registry.count <= 1) {
        return true;
    }

    DetectedEvent* const* arr = registry.events;
    for (int i = 1; i < registry.count; ++i) {
        if (*(*(arr + i)) < *(*(arr + i - 1))) {
            return false;
        }
    }
    return true;
}

bool EventSorter::isSorted(const EventRegistry& registry, const EventComparator& comparator) {
    if (registry.count <= 1) {
        return true;
    }

    DetectedEvent* const* arr = registry.events;
    for (int i = 1; i < registry.count; ++i) {
        if (comparator(*(*(arr + i)), *(*(arr + i - 1)))) {
            return false;
        }
    }
    return true;
}
