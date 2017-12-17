#!/bin/bash
cd code/build.linux
echo "Rebuild Nachos"
make clean
make

cd ../
cd test
make clean
make
../build.linux/nachos -ep consoleIO_test1 100 -ep consoleIO_test2 99

