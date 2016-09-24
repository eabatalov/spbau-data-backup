#!/bin/bash
rm -f -r ./tests/unpacked/*
rm -f ./tests/archives/*.pck
cd ./bin
./test_archiver -p ../tests/1 ../tests/archives/1.pck
./test_archiver -u ../tests/archives/1.pck ../tests/unpacked/
./test_archiver -c ../tests/1 ../tests/unpacked/1 

./test_archiver -p ../tests/2 ../tests/archives/2.pck
./test_archiver -u ../tests/archives/2.pck ../tests/unpacked/
./test_archiver -c ../tests/2 ../tests/unpacked/2 

./test_archiver -p ../tests/3 ../tests/archives/3.pck
./test_archiver -u ../tests/archives/3.pck ../tests/unpacked/
./test_archiver -c ../tests/3 ../tests/unpacked/3 
