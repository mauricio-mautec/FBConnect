#!/bin/bash
set -e

clear
cd "$(dirname "$0")"
make clean
make
#valgrind --leak-check=yes ./FBConnectTest
#gdb ./FBConnectTest
./FBConnectTest
