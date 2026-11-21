CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -pedantic -std=c++17 -g -Iinclude

OBJS = build/MetadataSensor.o \
       build/DinamicArr.o \
       build/ChannelStadistics.o \
       build/ChannelSignal.o \
       build/SlicingWindow.o

.PHONY: all test valgrind app clean

all: build/test_fase1 build/app_final

build:
	mkdir -p build

build/MetadataSensor.o: src/MetadataSensor.cpp include/MetadataSensor.h | build
	$(CXX) $(CXXFLAGS) -c src/MetadataSensor.cpp -o build/MetadataSensor.o

build/DinamicArr.o: src/DinamicArr.cpp include/DinamicArr.h | build
	$(CXX) $(CXXFLAGS) -c src/DinamicArr.cpp -o build/DinamicArr.o

build/ChannelStadistics.o: src/ChannelStadistics.cpp include/ChannelStadistics.h | build
	$(CXX) $(CXXFLAGS) -c src/ChannelStadistics.cpp -o build/ChannelStadistics.o

build/ChannelSignal.o: src/ChannelSignal.cpp include/ChannelSignal.h include/DinamicArr.h include/MetadataSensor.h include/ChannelStadistics.h | build
	$(CXX) $(CXXFLAGS) -c src/ChannelSignal.cpp -o build/ChannelSignal.o

build/SlicingWindow.o: src/SlicingWindow.cpp include/SlicingWindow.h include/ChannelSignal.h | build
	$(CXX) $(CXXFLAGS) -c src/SlicingWindow.cpp -o build/SlicingWindow.o

build/main_fase1.o: tests/main_fase1.cpp include/MetadataSensor.h include/DinamicArr.h include/ChannelStadistics.h include/ChannelSignal.h include/SlicingWindow.h | build
	$(CXX) $(CXXFLAGS) -c tests/main_fase1.cpp -o build/main_fase1.o

build/main_app.o: app/main.cpp include/ChannelSignal.h include/SlicingWindow.h | build
	$(CXX) $(CXXFLAGS) -c app/main.cpp -o build/main_app.o

build/test_fase1: $(OBJS) build/main_fase1.o
	$(CXX) $(CXXFLAGS) $(OBJS) build/main_fase1.o -o build/test_fase1

build/app_final: $(OBJS) build/main_app.o
	$(CXX) $(CXXFLAGS) $(OBJS) build/main_app.o -o build/app_final

test: build/test_fase1
	./build/test_fase1

valgrind: build/test_fase1
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./build/test_fase1

app: build/app_final
	./build/app_final

clean:
	rm -f build/*.o build/test_fase1 build/app_final
