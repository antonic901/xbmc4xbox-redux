#!/bin/sh
rm -rf build
mkdir -p build
cd build
cmake -G "MSYS Makefiles" -DWITH_JPEG8=1 ../src
make
cd ..
cp build/libturbojpeg.a lib/libturbojpeg.lib
cp build/jconfig.h include
cp src/jmorecfg.h include
cp src/jpeglib.h include
