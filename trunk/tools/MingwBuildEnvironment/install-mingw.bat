xcopy files\* C:\MinGW\ /E /I /Y
set PATH=%PATH%;C:\MinGW\bin;C:\MinGW\MSYS\1.0\local\bin;C:\MinGW\msys\1.0\bin
mingw-get update
copy files\var\lib\mingw-get\data\*.xml C:\MinGW\var\lib\mingw-get\data\ /Y
mingw-get install gcc
mingw-get install libz
mingw-get install msys
mingw-get install msys-coreutils
mingw-get install msys-patch
mingw-get install msys-diffutils
mingw-get install msys-automake

:: get yasm
mingw-get install msys-wget
mkdir C:\MinGW\msys\1.0\local\bin
wget http://www.tortall.net/projects/yasm/releases/yasm-1.2.0-win32.exe -O C:/MinGW/MSYS/1.0/local/bin/yasm.exe

pause