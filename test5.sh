#!/bin/bash
cd code/build.linux
make clean
make

cd ../test
make clean
make
../build.linux/nachos -ep consoleIO_test2 33 -ep consoleIO_test3 44
