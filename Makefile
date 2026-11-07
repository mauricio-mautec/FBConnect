CXX      := g++
CXXFLAGS := -Wall -Wextra -ggdb -I/usr/include/firebird
LDFLAGS  := -lfbclient

TARGET   := FBConnectTest
OBJS     := FBConnect.o FBConnectTest.o

.PHONY: all clean run valgrind

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

FBConnect.o: FBConnect.cpp FBConnect.h
	$(CXX) $(CXXFLAGS) -c FBConnect.cpp -o $@

FBConnectTest.o: FBConnectTest.cpp FBConnect.h
	$(CXX) $(CXXFLAGS) -c FBConnectTest.cpp -o $@

run: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full ./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) a.out

