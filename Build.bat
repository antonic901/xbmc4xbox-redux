@ECHO OFF
rem CLS
COLOR 1B
TITLE XBMC Build Prepare Script
rem ----PURPOSE----
rem - Create a working XBMC build with a single click
rem -------------------------------------------------------------
rem Usage: built.bat [noprompt] [nocompress] [noclean] [build1-4]
rem -------------------------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem ONLY needed if you have a very old bios, SET the path for xbepatch. Not needed otherwise.
rem -------------------------------------------------------------
rem CONFIG START

SET XBE_PATCH=tools\xbepatch\xbepatch.exe

SET COMPRESS=C:\Program Files\7-zip\7z.exe

SET Silent=0
SET SkipCompression=0
SET Clean=1
SET Compile=1

IF "%VS71COMNTOOLS%"=="" (
  SET NET="%ProgramFiles%\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv.com"
) ELSE (
  SET NET="%VS71COMNTOOLS%\..\IDE\devenv.com"
)

IF NOT EXIST %NET% (
  CALL:ERROR "Visual Studio .NET 2003 was not found."
  GOTO:EOF
)

:GETPARAMS
  set IN=%1
  IF "%IN%" EQU "noprompt" (
    SET Silent=1
  ) ELSE IF "%IN%" EQU "nocompress" (
    SET SkipCompression=1
  ) ELSE IF "%IN%" EQU "noclean" (
    SET Clean=0
  ) ELSE IF "%IN:~0,5%" EQU "build" (
    SET XBMC_COMPILE_ANSWER=%IN:~5,1%
    SET Silent=1
  ) ELSE IF "%IN%" EQU "" (
    GOTO ENDPARAMS
  )
  SHIFT
  GOTO GETPARAMS
:ENDPARAMS

IF %Silent% EQU 0 (
  CALL:MENU
)

SET DEST=BUILD

IF %XBMC_COMPILE_ANSWER% EQU 1 (
  SET VS_PATH=.
  SET VS_SOL=xbmc.sln
  SET VS_CONF=Release
  SET VS_BIN=default.xbe
)

IF %XBMC_COMPILE_ANSWER% EQU 2 (
  SET VS_PATH=.
  SET VS_SOL=xbmc.sln
  SET VS_CONF=Release_LTCG
  SET VS_BIN=default.xbe
)

IF %XBMC_COMPILE_ANSWER% EQU 3 (
  SET VS_PATH=.
  SET VS_SOL=xbmc.sln
  SET VS_CONF=Debug
  SET VS_BIN=default.xbe
)

IF %XBMC_COMPILE_ANSWER% EQU 4 (
  SET DEST=BUILD_WIN32
  SET VS_PATH=tools\Win32
  SET VS_SOL=XBMC_PC.sln
  SET VS_CONF=Release
  SET VS_BIN=XBMC_PC.exe
)

IF %XBMC_COMPILE_ANSWER% EQU "" GOTO:EOF

IF %Silent% EQU 0 (
  IF EXIST %VS_PATH%\%VS_CONF%\%VS_BIN% (
    CALL:BIN_EXISTS
  )
)

IF %Compile% EQU 1 (
  CALL:COMPILE
)

CALL:MAKE_BUILD %DEST%

ECHO ------------------------------------------------------------
ECHO Build Succeeded!
IF %SkipCompression%==0 (
  CALL:COMPRESS %DEST%
)

IF %Silent% EQU 0 (
  CALL:VIEWLOG
)

pause
GOTO:EOF

:MENU
  ECHO    ВВВВВВВББББББББААААААА
  ECHO  ВлллллллллллллллллллллллВВВВВВБББББАААААА     пппВмм
  ECHO олллллллллллллллллллллллллллллллллллллллллВВВВБББАА  ппм
  ECHO ВлллллллллллллллллллллллллллллллллллллллллллллллллллВА  н
  ECHO ВлллллллллллллллллллплллллллллллллллллллллллллллллллллА В
  ECHO БллллллллллллллллллнАлллллллллллллллллллллллллллллллллл о
  ECHO АллллллВБА  плп           плллп    пВп    плллп   АВллл о
  ECHO  ллллллллллн   млллн Влллм олн мВлм   мллм ол  мллллллл о
  ECHO  Влллллллллл  лллллн лллллн л оллллн оллллн н олллллллл В
  ECHO  Блллллллллн олллллн лллллн л лллллн лллллн   ВлллллллВ н
  ECHO  АВлллллллп   пллллн плллп он лллллн лллллн А плллллллн н
  ECHO   БлллВБА мллм АБВллм     млВмллллллмлллллВ лм   АВлллно
  ECHO   АВлллллллллллллллллллллллллллллллллллллллллллллллллл В
  ECHO    Блллллпппплпплппллпллпллллпплпплппплпплпплппллллллл н
  ECHO    АВлллл но л пл л л лнмоллл лл пл л лнол пл пмлллллБ н
  ECHO     Блллл но л пл пмл л м ллл пл пл л лнол пл л лллллАо
  ECHO     АВллллллллллллллллллллллллллллллллллллллллллллллВ
  ECHO      БАммммммммммммммммммммммммммммммммммммм  АБВВВВ
  ECHO      АВллллллллллллллллллллллллллллллллллллллВААБВп
  ECHO       БВлллллллллллллллллллллллллллллВлВВпппп
  ECHO        ВВллллллллллллллллллВлВВпппп
  ECHO         пВллллВлВВВпппппп
  ECHO ------------------------------------------------------------
  ECHO XBMC prepare menu
  ECHO ------------------------------------------------------------
  ECHO [1] Build XBMC XBE      ( for XBOX use )
  ECHO [2] Build LTCG XBMC XBE ( for XBOX use )
  ECHO [3] Build DEBUG XBE     ( for XBOX use )
  ECHO [4] Build XBMC_WIN32    ( for Windows use)
  ECHO ------------------------------------------------------------
  SET /P XBMC_COMPILE_ANSWER=Please enter the number you want to build [1/2/3/4]:
  GOTO:EOF

:BIN_EXISTS
  ECHO ------------------------------------------------------------
  ECHO Found a previous Compiled binary - %VS_PATH%\%VS_CONF%\%VS_BIN% !
  ECHO [1] a NEW binary will be compiled for the BUILD 
  ECHO [2] existing binary will be updated (quick mode compile) for the BUILD
  ECHO [3] existing binary will be used for the BUILD 
  ECHO ------------------------------------------------------------
  SET /P XBMC_COMPILE_ANSWER=Compile a new binary? [1/2/3]:
  IF /I %XBMC_COMPILE_ANSWER% EQU 1 SET Clean=1
  IF /I %XBMC_COMPILE_ANSWER% EQU 2 SET Clean=0
  IF /I %XBMC_COMPILE_ANSWER% EQU 3 SET Compile=0
  GOTO:EOF
  
:COMPILE
  ECHO Wait while preparing the build.
  ECHO ------------------------------------------------------------
  IF %Clean% EQU 1 (
    ECHO Cleaning Solution...
    %NET% %VS_PATH%\%VS_SOL% /clean %VS_CONF%
    DEL %VS_PATH%\%VS_CONF%\xbmc.map 2>NUL
  )
  ECHO Compiling Solution...
  %NET% %VS_PATH%\%VS_SOL% /build %VS_CONF%
  IF NOT EXIST %VS_PATH%\%VS_CONF%\%VS_BIN% (
    CALL:ERROR "%VS_BIN% failed to build!  See .\%VS_CONF%\BuildLog.htm for details."
    CALL:VIEWLOG
    PAUSE
    EXIT
  )
  ECHO Done!
  ECHO ------------------------------------------------------------
  GOTO:EOF

:MAKE_BUILD

  IF EXIST %~1 RMDIR %~1 /S /Q
  MKDIR %~1
  
  ECHO Copying files to %~1 ...
  
  xcopy /Y %VS_PATH%\%VS_CONF%\%VS_BIN% %~1
  
  IF "%~1" EQU "BUILD" (
    ECHO - XBE Patching %VS_PATH%\%VS_CONF%\%VS_BIN%
    %XBE_PATCH% %~1\%VS_BIN%
    ECHO - Patching Done!
  )
  
  ECHO .svn>exclude.txt
  ECHO Thumbs.db>>exclude.txt
  ECHO Desktop.ini>>exclude.txt
  ECHO dsstdfx.bin>>exclude.txt
  ECHO Splash_2007.png>>exclude.txt
  ECHO Splash_2008.png>>exclude.txt
  ECHO Splash_2014.png>>exclude.txt
  ECHO exclude.txt>>exclude.txt
  ECHO addons\skin >>exclude.txt

  mkdir %~1\home
  xcopy userdata %~1\home\userdata /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy *.txt %~1 /EXCLUDE:exclude.txt

  SET RUN_ME=%~1\run_me.bat
  IF "%~1" EQU "BUILD_WIN32" (
    ECHO subst q: .>%RUN_ME%
    ECHO subst p: q:\userdata >>%RUN_ME%
    ECHO subst t: q:\userdata >>%RUN_ME%
    ECHO if not exist q:\Temp md Temp >>%RUN_ME%
    ECHO subst z: Temp >>%RUN_ME%
    ECHO XBMC_PC.exe >>%RUN_ME%
    ECHO subst z: /D >>%RUN_ME%
    ECHO subst t: /D >>%RUN_ME%
    ECHO subst p: /D >>%RUN_ME%
    ECHO subst q: /D >>%RUN_ME%
  )

  cd "addons/skin.estuary"
  CALL build.bat
  cd ..\..
  xcopy "addons/skin.estuary\BUILD\skin.estuary" "%~1\addons\skin.estuary" /E /Q /I /Y /EXCLUDE:exclude.txt

  
  xcopy addons %~1\addons /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy system %~1\system /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy media   %~1\media   /E /Q /I /Y /EXCLUDE:exclude.txt
  IF EXIST updater.xbe (
	xcopy updater.xbe %~1 /Y /Q
  )
  
  del exclude.txt
  GOTO:EOF


:COMPRESS
  ECHO ------------------------------------------------------------
  ECHO Compressing build to XBMC4XBOX.zip file...
  ECHO ------------------------------------------------------------
  IF EXIST "%COMPRESS%" (
    DEL XBMC4Xbox.zip
	DEL XBMC4Xbox.tar
    "%COMPRESS%" a XBMC4Xbox.zip "%~1"
	"%COMPRESS%" a XBMC4Xbox.tar "%~1"
  ) ELSE ( 
    ECHO 7-Zip not installed!  Skipping compression...
  )
  ECHO ------------------------------------------------------------
  GOTO:EOF
  
:ERROR
  ECHO ------------------------------------------------------------
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  ECHO    ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  ECHO ERROR %~1
  ECHO ------------------------------------------------------------
  GOTO:EOF
  
:VIEWLOG
  SET /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
  if /I "%XBMC_BUILD_ANSWER%" EQU "y" (
    start /D"%~dp0%VS_PATH%\%VS_CONF%" BuildLog.htm"
  )
  GOTO:EOF
