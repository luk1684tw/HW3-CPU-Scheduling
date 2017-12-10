#!/bin/bash
cd code/build.linux
echo "Rebulid Nachos"
make clean
make

cd ../
cd test
make clean
make
../build.linux/nachos -e consoleIO_test1 -e consoleIO_test2

