#!/bin/bash

cd "$(dirname "$0")"

if [ ! -d "build" ]; then
	echo "Creating build directory..."
	mkdir build
	cd build
	cmake .. -DCMAKE_TOOLCHAIN_FILE="../vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
else
	echo "Using existing build directory..."
	cd build
fi

cmake --build . --config Release
