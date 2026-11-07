CXX      := g++
CXXFLAGS := -Wall -Wextra -ggdb -I/usr/include/firebird
LDFLAGS  := -lfbclient -lncursesw

TARGETS  := FBConnectTest exemploFBConnect
OBJS     := FBConnect.o MauWin.o FBConnectTest.o exemploFBConnect.o

.PHONY: all clean run run-exemplo valgrind

all: $(TARGETS)

FBConnectTest: FBConnect.o MauWin.o FBConnectTest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

exemploFBConnect: FBConnect.o MauWin.o exemploFBConnect.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

FBConnect.o: FBConnect.cpp FBConnect.h
	$(CXX) $(CXXFLAGS) -c FBConnect.cpp -o $@

MauWin.o: MauWin.cpp MauWin.h
	$(CXX) $(CXXFLAGS) -c MauWin.cpp -o $@

FBConnectTest.o: FBConnectTest.cpp FBConnect.h MauWin.h
	$(CXX) $(CXXFLAGS) -c FBConnectTest.cpp -o $@

exemploFBConnect.o: exemploFBConnect.cpp FBConnect.h MauWin.h
	$(CXX) $(CXXFLAGS) -c exemploFBConnect.cpp -o $@

run: FBConnectTest
	./FBConnectTest

run-exemplo: exemploFBConnect
	./exemploFBConnect

valgrind: FBConnectTest
	valgrind --leak-check=full ./FBConnectTest --batch

clean:
	rm -f $(OBJS) $(TARGETS) a.out
