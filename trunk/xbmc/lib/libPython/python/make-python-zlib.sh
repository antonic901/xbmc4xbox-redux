#!/bin/sh
cd Lib/
zip ../python24.zlib \
  -r . \
  -i \*.py \
  -x \
    plat-\* \
    distutils/\* \
    curses/\* \
    lib-tk/\* \
    idlelib/\* \
    test/\*
