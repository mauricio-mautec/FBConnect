#!/bin/bash
set -e

cd "$(dirname "$0")"
make clean
make
#valgrind --leak-check=yes ./FBConnectTest
#gdb ./FBConnectTest
./FBConnectTest
