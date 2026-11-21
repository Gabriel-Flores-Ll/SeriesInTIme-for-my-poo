#ifndef SLICING_WINDOW_H
#define SLICING_WINDOW_H

#include <string>

class ChannelSignal;

/**
 * @class SlicingWindow
 * @brief Zero-copy read-only sliding window associated with an external ChannelSignal.
 *        Does not own the ChannelSignal memory and never copies sample buffers.
 * @invariant When channelAsocied != nullptr: 1 <= windowLenght <= lengthOfTheChannel.
 * @invariant stride >= 1.
 * @invariant When channelAsocied != nullptr: 0 <= initIndex and initIndex + windowLenght <= lengthOfTheChannel.
 */
class SlicingWindow {
private:
    const ChannelSignal* channelAsocied;
    int lengthOfTheChannel;
    int initIndex;
    int windowLenght;
    int stride;

public:
    /**
     * @brief Constructs a sliding window bound to a read-only ChannelSignal.
     * @param channelProvided Non-owning pointer to the target channel (guarded against nullptr).
     * @param windowLenghtProvided Window size W in samples (clamped to [1, N] if channel is valid).
     * @param strideProvided Step size S in samples (clamped to >= 1).
     */
    SlicingWindow(const ChannelSignal* channelProvided, int windowLenghtProvided, int strideProvided);

    /**
     * @brief Resets the window start index back to 0.
     */
    void resetInitIndex();

    /**
     * @brief Moves the window start index to indexProvided if the window stays within channel bounds.
     * @param indexProvided Candidate starting index.
     * @return true if repositioned, false if out of bounds or detached.
     */
    bool setInitIndex(int indexProvided);

    /**
     * @brief Checks whether another full stride step fits inside the channel bounds.
     * @return true if initIndex + stride + windowLenght <= lengthOfTheChannel, false otherwise.
     */
    bool canStrideForward() const;

    /**
     * @brief Advances the window by stride samples if the resulting window stays inside bounds.
     * @return true if advanced, false if no further stride fits.
     */
    bool strideForward();

    /**
     * @brief Indicates whether the window can no longer advance by a full stride (safe loop termination check).
     * @return true if no further stride is possible, false otherwise.
     */
    bool itsInTheBorder() const;

    int getInitIndex() const;
    int getWindowLenght() const;
    int getStride() const;

    /**
     * @brief Computes the temporal duration of the window in seconds (W / fs).
     * @return Window duration in seconds, or 0.0 if detached.
     */
    double calcWindowInSecs() const;

    /**
     * @brief Computes the start timestamp of the current window position in seconds (initIndex / fs).
     * @return Start time in seconds, or 0.0 if detached.
     */
    double calcStartTimeInSecs() const;

    /**
     * @brief Delegates statistical computations over the active window to ChannelSignal (Tell, Don't Ask).
     * @param selectionOfArr Buffer selector: "raw" or "processed".
     * @return Requested window metric, or quiet_NaN() if detached or selector is invalid.
     */
    double maxWindow(const std::string& selectionOfArr) const;
    double minWindow(const std::string& selectionOfArr) const;
    double energyWindow(const std::string& selectionOfArr) const;
    double averageWindow(const std::string& selectionOfArr) const;
};

#endif
