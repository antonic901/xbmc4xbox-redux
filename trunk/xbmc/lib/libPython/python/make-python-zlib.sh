#!/bin/sh
OUT="../../../../../system/python/python27.zlib"
cd Lib/
rm "$OUT"
zip ../../../../../system/python/python27.zlib \
  -r . \
  -i \*.py \
  -x \
    plat-\* \
    distutils/\* \
    curses/\* \
    lib-tk/\* \
    lib2to3/\* \
    idlelib/\* \
    test/\* \
    unittest/\* \
    multiprocessing/\* \
    \*/tests/\* \
    \*/test/\*
