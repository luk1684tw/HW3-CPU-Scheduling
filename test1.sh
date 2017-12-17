#!/bin/bash
cd code/build.linux
echo "Rebuild NachOS"
make clean
make

cd ../test
make clean
make
../build.linux/nachos -ep consoleIO_test5 101 -ep consoleIO_test2 155
