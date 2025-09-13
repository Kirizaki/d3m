mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH="/Users/greg/Qt/6.9.2/macos/lib/cmake/Qt6" ..
make -j$(sysctl -n hw.ncpu)
