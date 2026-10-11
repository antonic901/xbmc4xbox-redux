#!/bin/sh
DEST="../../system/python"
OUT="$DEST/python34.zlib"

cp -v "python/XBOXbuild/Release/python34.dll" "$DEST/"
cp -v "python/XBOXbuild/Release/"*.pyd "$DEST/DLLs"
rm "$OUT"

cd python/Lib/
zip "../../$OUT" \
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

zip "../../$OUT" \
  distutils/version.py
