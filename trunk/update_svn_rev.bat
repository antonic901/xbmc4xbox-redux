@echo off
REM subwcrev is included in the tortoise svn client: http://tortoisesvn.net/downloads

REM Current directory including drive
SET CWD=%~dp0

SET REV_FILE="%CWD%xbmc\xbox\svn_rev.h"
SET SVN_TEMPLATE="%CWD%xbmc\xbox\svn_rev.tmpl"

IF EXIST %REV_FILE% del %REV_FILE%

SET SUBWCREV=""

IF EXIST "%ProgramFiles(x86)%\TortoiseSVN\bin\subwcrev.exe" SET SUBWCREV="%ProgramFiles(x86)%\TortoiseSVN\bin\subwcrev.exe"
IF EXIST "%ProgramFiles%\TortoiseSVN\bin\subwcrev.exe"      SET SUBWCREV="%ProgramFiles%\TortoiseSVN\bin\subwcrev.exe"
IF EXIST "%ProgramW6432%\TortoiseSVN\bin\subwcrev.exe" SET SUBWCREV="%ProgramW6432%\TortoiseSVN\bin\subwcrev.exe"

IF NOT EXIST %SUBWCREV% (
   ECHO subwcrev.exe not found in expected locations, skipping generation
   GOTO SKIPSUBWCREV
)

%SUBWCREV% %SVN_TEMPLATE% %REV_FILE% -f

REM Generate the SVN revision header if it does not exist
IF NOT EXIST %REV_FILE% (
   ECHO Generating SVN revision header from SVN repo
   %SUBWCREV% "%CWD%." %SVN_TEMPLATE% %REV_FILE% -f
)

:SKIPSUBWCREV

REM Copy the default unknown revision header if the generation did not occur
IF NOT EXIST %REV_FILE% (
   ECHO using default svn revision unknown header
   copy %CWD%xbmc\xbox\svn_rev.unknown %REV_FILE%
)

SET REV_FILE=
SET SUBWCREV=
