#!/bin/bash
cd code/build.linux
echo "Rebuild NachOS"
make clean
make

cd ../
cd test
make clean
make
../build.linux/nachos -ep consoleIO_test5 51 -ep consoleIO_test6 49
