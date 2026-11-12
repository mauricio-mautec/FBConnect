#!/bin/bash
set -e

cd "$(dirname "$0")"
[ -f .fbconnect.env ] && . ./.fbconnect.env
make clean
make
#valgrind --leak-check=yes ./FBConnectTest
#gdb ./FBConnectTest
./FBConnectTest
