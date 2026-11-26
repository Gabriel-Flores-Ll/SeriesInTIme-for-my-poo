CXX      := g++
CXXFLAGS := -Wall -Wextra -Werror -pedantic -std=c++17 -g -Iinclude
VALGRIND := valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1

BUILD_DIR := build

FASE1_SRCS := src/DinamicArr.cpp \
              src/MetadataSensor.cpp \
              src/ChannelStadistics.cpp \
              src/ChannelSignal.cpp \
              src/SlicingWindow.cpp

FASE2_SRCS := src/ProcessingModule.cpp \
              src/WindowFilter.cpp \
              src/PointTransformer.cpp \
              src/MovingAverageFilter.cpp \
              src/MedianFilter.cpp \
              src/DifferentialFilter.cpp \
              src/WaveRectifier.cpp \
              src/ThresholdLimiter.cpp \
              src/ProcessingPipeline.cpp

FASE3_SRCS := src/DetectecEvent.cpp \
              src/PeakEvent.cpp \
              src/ThresholdEvent.cpp \
              src/StatisticalAnomalyEvent.cpp \
              src/EventRegistry.cpp \
              src/EventDetector.cpp \
              src/PeakDetector.cpp \
              src/ThresholdDetector.cpp \
              src/WindowAnomalyDetector.cpp \
              src/DetectionEngine.cpp

FASE4_SRCS := src/EventComparator.cpp \
              src/SeverityComparator.cpp \
              src/ChronologicalComparator.cpp \
              src/DurationComparator.cpp \
              src/ChannelComparator.cpp \
              src/EventSorter.cpp

FASE1_OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(FASE1_SRCS))
FASE2_OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(FASE2_SRCS))
FASE3_OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(FASE3_SRCS))
FASE4_OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(FASE4_SRCS))

TEST_F1_OBJ := $(BUILD_DIR)/main_fase1.o
TEST_F2_OBJ := $(BUILD_DIR)/main_fase2.o
TEST_F3_OBJ := $(BUILD_DIR)/main_fase3.o
TEST_F4_OBJ := $(BUILD_DIR)/main_fase4.o

TARGET_F1 := $(BUILD_DIR)/test_fase1
TARGET_F2 := $(BUILD_DIR)/test_fase2
TARGET_F3 := $(BUILD_DIR)/test_fase3
TARGET_F4 := $(BUILD_DIR)/test_fase4

.PHONY: all fase1 fase2 fase3 fase4 \
        run-fase1 run-fase2 run-fase3 run-fase4 run-all \
        valgrind-fase1 valgrind-fase2 valgrind-fase3 valgrind-fase4 valgrind-all clean

all: $(TARGET_F1) $(TARGET_F2) $(TARGET_F3) $(TARGET_F4)

fase1: $(TARGET_F1)

fase2: $(TARGET_F2)

fase3: $(TARGET_F3)

fase4: $(TARGET_F4)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: tests/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET_F1): $(FASE1_OBJS) $(FASE2_OBJS) $(TEST_F1_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TARGET_F2): $(FASE1_OBJS) $(FASE2_OBJS) $(TEST_F2_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TARGET_F3): $(FASE1_OBJS) $(FASE2_OBJS) $(FASE3_OBJS) $(FASE4_OBJS) $(TEST_F3_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TARGET_F4): $(FASE1_OBJS) $(FASE2_OBJS) $(FASE3_OBJS) $(FASE4_OBJS) $(TEST_F4_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

run-fase1: $(TARGET_F1)
	./$(TARGET_F1)

run-fase2: $(TARGET_F2)
	./$(TARGET_F2)

run-fase3: $(TARGET_F3)
	./$(TARGET_F3)

run-fase4: $(TARGET_F4)
	./$(TARGET_F4)

run-all: run-fase1 run-fase2 run-fase3 run-fase4

valgrind-fase1: $(TARGET_F1)
	$(VALGRIND) ./$(TARGET_F1)

valgrind-fase2: $(TARGET_F2)
	$(VALGRIND) ./$(TARGET_F2)

valgrind-fase3: $(TARGET_F3)
	$(VALGRIND) ./$(TARGET_F3)

valgrind-fase4: $(TARGET_F4)
	$(VALGRIND) ./$(TARGET_F4)

valgrind-all: valgrind-fase1 valgrind-fase2 valgrind-fase3 valgrind-fase4

clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET_F1) $(TARGET_F2) $(TARGET_F3) $(TARGET_F4)
