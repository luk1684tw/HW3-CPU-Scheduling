#!/bin/bash
cd code/build.linux
echo "Rebuild NachOS"
make clean
make

cd ../test
make clean
make
../build.linux/nachos -ep consoleIO_test3 99 -ep consoleIO_test4 51
