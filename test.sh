#!/bin/bash
set -e

clear
cd "$(dirname "$0")"
[ -f .fbconnect.env ] && . ./.fbconnect.env
make clean
make
#valgrind --leak-check=yes ./FBConnectTest
#gdb ./FBConnectTest
./FBConnectTest
