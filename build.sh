#!/bin/sh
mkdir -p build
pushd build > /dev/null
rm -rf sdl_platform.dSYM

# Plugin library
c++ -Wall -std=c++11 -c -fpic -g ../code/rt_weekend.cpp -o librt_weekend.o
c++ -shared -o librt_weekend.so librt_weekend.o
rm librt_weekend.o

# Platform layer host application
c++ -Wall -std=c++11 -g ../code/platforms/sdl_platform.cpp -o sdl_platform \
`sdl2-config --libs --cflags`

popd > /dev/null
