CXX      := g++
CXXFLAGS := -Wall -Wextra -Werror -pedantic -std=c++17 -g -Iinclude
VALGRIND := valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1

BUILD_DIR := build

FASE1_SRCS := src/DinamicArr.cpp \
              src/MetadataSensor.cpp \
              src/ChannelStadistics.cpp \
              src/ChannelSignal.cpp \
              src/SlicingWindow.cpp

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

FASE1_OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(FASE1_SRCS))
FASE3_OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(FASE3_SRCS))

TEST_F1_OBJ := $(BUILD_DIR)/main_fase1.o
TEST_F3_OBJ := $(BUILD_DIR)/main_fase3.o

TARGET_F1 := $(BUILD_DIR)/test_fase1
TARGET_F3 := $(BUILD_DIR)/test_fase3

.PHONY: all fase1 fase3 run-fase1 run-fase3 run-all valgrind-fase1 valgrind-fase3 valgrind-all clean

all: $(TARGET_F1) $(TARGET_F3)

fase1: $(TARGET_F1)

fase3: $(TARGET_F3)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: tests/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET_F1): $(FASE1_OBJS) $(TEST_F1_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TARGET_F3): $(FASE1_OBJS) $(FASE3_OBJS) $(TEST_F3_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

run-fase1: $(TARGET_F1)
	./$(TARGET_F1)

run-fase3: $(TARGET_F3)
	./$(TARGET_F3)

run-all: run-fase1 run-fase3

valgrind-fase1: $(TARGET_F1)
	$(VALGRIND) ./$(TARGET_F1)

valgrind-fase3: $(TARGET_F3)
	$(VALGRIND) ./$(TARGET_F3)

valgrind-all: valgrind-fase1 valgrind-fase3

clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET_F1) $(TARGET_F3)