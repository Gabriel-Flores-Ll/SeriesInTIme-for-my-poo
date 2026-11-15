CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -pedantic -std=c++17 -g -Iinclude

build/MetadataSensor.o: src/MetadataSensor.cpp include/MetadataSensor.h
	$(CXX) $(CXXFLAGS) -c src/MetadataSensor.cpp -o build/MetadataSensor.o

build/ChannelStadistics.o: src/ChannelStadistics.cpp include/ChannelStadistics.h
	$(CXX) $(CXXFLAGS) -c src/ChannelStadistics.cpp -o build/ChannelStadistics.o

build/main_fase1.o: tests/main_fase1.cpp include/MetadataSensor.h include/ChannelStadistics.h
	$(CXX) $(CXXFLAGS) -c tests/main_fase1.cpp -o build/main_fase1.o

build/main_app.o: app/main.cpp include/MetadataSensor.h include/ChannelStadistics.h
	$(CXX) $(CXXFLAGS) -c app/main.cpp -o build/main_app.o

build/test_fase1: build/MetadataSensor.o build/ChannelStadistics.o build/main_fase1.o
	$(CXX) $(CXXFLAGS) build/MetadataSensor.o build/ChannelStadistics.o build/main_fase1.o -o build/test_fase1

build/finalProgram: build/MetadataSensor.o build/ChannelStadistics.o build/main_app.o
	$(CXX) $(CXXFLAGS) build/MetadataSensor.o build/ChannelStadistics.o build/main_app.o -o build/finalProgram

test: build/test_fase1
	./build/test_fase1

finalProgram : build/finalProgram
	./build/finalProgram

clean:
	rm -f build/*.o build/test_fase1 build/app_final
