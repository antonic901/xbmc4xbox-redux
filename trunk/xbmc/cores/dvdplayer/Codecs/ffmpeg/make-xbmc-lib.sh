#!/bin/bash 

# This script should be could from eg. an MSYS/MinGW (compile) environment
# Don't forget to setup msys.bat properly!

rm -r .libs
make distclean

./configure \
--extra-cflags="-D_XBOX -fno-common -mtune=pentium3 -msse -ffast-math -mfpmath=sse -pipe -Wno-unused-function" \
--cpu=pentium3 \
--enable-gpl \
--enable-shared \
--disable-static \
--enable-w32threads \
--enable-memalign-hack \
--enable-small \
--enable-zlib \
--disable-debug \
--disable-ipv6 \
\
--disable-ffmpeg \
--disable-ffplay \
--disable-ffserver \
\
--enable-swscale \
--disable-vhook \
--disable-muxers \
--disable-encoders \
\
--enable-postproc \
\
--disable-filters \
--enable-filter=buffer \
\
--disable-bsfs \
--disable-indevs \
--disable-vdpau \
\
--disable-altivec \
--disable-amd3dnow \
--disable-amd3dnowext \
--disable-ssse3 \
--disable-armv5te \
--disable-armv6 \
--disable-armv6t2 \
--disable-armvfp \
--disable-iwmmxt \
--disable-neon \
--disable-vis &&
 
make -j3 && 
mkdir .libs &&
cp lib*/*.dll .libs/ &&
mv .libs/swscale-0.dll .libs/swscale-0.6.1.dll &&
cp -v .libs/avcodec-52.dll ../../../../../system/players/dvdplayer/ &&
cp -v .libs/avformat-52.dll ../../../../../system/players/dvdplayer/ &&
cp -v .libs/avutil-49.dll ../../../../../system/players/dvdplayer/ &&
cp -v .libs/postproc-51.dll ../../../../../system/players/dvdplayer/ &&
cp -v .libs/swscale-0.6.1.dll ../../../../../system/players/dvdplayer/
