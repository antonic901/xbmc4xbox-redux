@echo off
rem subwcrev is included in the tortoise svn client: http://tortoisesvn.net/downloads

SET REV_FILE=..\xbmc\xbox\svn_rev.h

IF EXIST %REV_FILE% (
  del %REV_FILE%
)

IF EXIST "%ProgramFiles(x86)%\TortoiseSVN\bin\subwcrev.exe" SET SUBWCREV="%ProgramFiles(x86)%\TortoiseSVN\bin\subwcrev.exe"
IF EXIST "%ProgramFiles%\TortoiseSVN\bin\subwcrev.exe"      SET SUBWCREV="%ProgramFiles%\TortoiseSVN\bin\subwcrev.exe"
IF EXIST "%ProgramFilesW6432%\TortoiseSVN\bin\subwcrev.exe" SET SUBWCREV="%ProgramFilesW6432%\TortoiseSVN\bin\subwcrev.exe"

IF NOT EXIST %SUBWCREV% (
  ECHO SubWCRev not found, unable to embed correct revision number. Check TortoiseSVN install.
  GOTO SKIPSUBWCREV
)

%SUBWCREV% .. ../xbmc/xbox/svn_rev.tmpl %REV_FILE% -f

IF NOT EXIST %REV_FILE% (
  %SUBWCREV% .. ../xbmc/xbox/svn_rev.tmpl %REV_FILE% -f
)

:SKIPSUBWCREV

IF NOT EXIST %REV_FILE% (
  copy ..\xbmc\xbox\svn_rev.unknown %REV_FILE%
)

SET REV_FILE=
SET SUBWCREV=
