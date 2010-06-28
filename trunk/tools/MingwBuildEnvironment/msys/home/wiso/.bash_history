cd c:
cd Development/xbmc_linux/XBMC/xbmc/cores/dvdplayer/Codecs/ffmpeg/
make distclean
./build_xbmc_win32.sh 
make --version
make
make clean
./build_xbmc_win32.sh 
make
strip lib*/*.dll && mkdir .libs && cp lib*/*.dll .libs/
