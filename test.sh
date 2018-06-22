#!/bin/bash
rm a.out
rm FBConnect.o

gcc -c -ggdb FBConnect.cpp -I/usr/include/firebird -lstdc++ -lfbclient
gcc FBConnectTest.cpp FBConnect.o -ggdb -I/usr/include/firebird -lstdc++ -lfbclient
#valgrind --leak-check=yes ./a.out
#gdb a.out
./a.out
