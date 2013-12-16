#!/bin/bash 

set -e

xbmc_clean ()
{
  echo "Cleaning ..."
  [ -d .libs ] && rm -r .libs
  make distclean 2>/dev/null || true
}

# $1 = additional configure parameters
xbmc_configure ()
{
  echo "Configuring ..."
  CFLAGS="-D_XBOX -msse -mfpmath=sse"
  LDFLAGS="-Wl,--enable-auto-import"
  PARAMS=" \
  --cpu=pentium3 \
  --enable-gpl \
  --enable-shared \
  --disable-static \
  --enable-w32threads \
  --enable-memalign-hack \
  --enable-small \
  --enable-zlib \
  --disable-debug \
  \
  --disable-doc \
  --disable-ffmpeg \
  --disable-ffplay \
  --disable-ffprobe \
  --disable-ffserver \
  \
  --disable-muxers \
  --enable-muxer=spdif,adts \
  --disable-encoders \
  --disable-devices \
  --disable-bsfs \
  \
  --enable-postproc \
  --disable-avdevice \
  --disable-avfilter \
  \
  --disable-protocol=rtmp,rtmpe,rtmps,rtmpt,rtmpte \
  \
  --disable-vdpau \
  --disable-vaapi \
  --disable-dxva2 \
  \
  --disable-altivec \
  --disable-amd3dnow \
  --disable-amd3dnowext \
  --disable-sse2 \
  --disable-sse3 \
  --disable-ssse3 \
  --disable-avx \
  --disable-fma4 \
  --disable-armv5te \
  --disable-armv6 \
  --disable-armv6t2 \
  --disable-vfp \
  --disable-neon \
  --disable-vis"
  echo "--extra-cflags=\"$CFLAGS\" --extra-ldflags=\"$LDFLAGS\" $PARAMS $1"
  ./configure --extra-cflags="$CFLAGS" --extra-ldflags="$LDFLAGS" $PARAMS $1
}

# $1 = destination folder
xbmc_make ()
{
  set -e
  echo "Making ..."
  make
  [ ! -d .libs ] && mkdir .libs
  cp lib*/*.dll .libs/
  if [ "$1" != "" ]; then
    echo "Copying libraries to $1 ..."
    [ ! -d "$1" ] && mkdir -p "$1"
    cp .libs/avcodec-54.dll "$1"
    cp .libs/avformat-54.dll "$1"
    cp .libs/avutil-52.dll "$1"
    cp .libs/postproc-52.dll "$1"
    cp .libs/swscale-2.dll "$1"
    cp .libs/swresample-0.dll "$1"
  fi
}

xbmc_all ()
{
  xbmc_clean
  xbmc_configure "\
    --disable-decoders \
    --enable-decoder=mpeg4,msmpeg4v1,msmpeg4v2,msmpeg4v3 \
    --enable-decoder=vp6,vp6a,vp6f,vp8 \
    --enable-decoder=mp1,mp2,mp3,mpegvideo,mpeg1video,mpeg2video \
    --enable-decoder=mjpeg,mjpegb,rawvideo \
    --enable-decoder=wmav1,wmav2,wmapro,wmv1,wmv2,wmv3 \
    --enable-decoder=aac,ac3,dca,dvbsub,dvdsub,flv,h263,h264,rtp,vorbis \
    \
    --disable-demuxers \
    --enable-demuxer=mp1,mp2,mp3,mpegps,mpegts,mpegtsraw,mpegvideo \
    --enable-demuxer=aac,ac3,dts,asf,avi,flv,h263,h264,ogg,matroska,mov \
    --enable-demuxer=nuv,sdp,rtsp,applehttp,xmv \
  "
  xbmc_make ../../../../../system/players/dvdplayer/
  xbmc_clean
  xbmc_configure
  xbmc_make ../../../../../system/players/dvdplayer/full
}

case "$1"
in
  clean)
    xbmc_clean
  ;;
  configure)
    xbmc_configure "$2"
  ;;
  make)
    xbmc_make "$2"
  ;;
  all)
    xbmc_all "$2"
  ;;
  *)
    echo "$0 clean|configure [additional parameters]|make [install dir]|all"
  ;;
esac

