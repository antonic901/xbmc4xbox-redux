@echo off

REM Current directory including drive
SET CWD=%~dp0

SET REV_FILE="%CWD%xbmc\xbox\svn_rev.h"

REM Remove existing revision file if it exists
IF EXIST %REV_FILE% del %REV_FILE%

REM Set SVN_REV
SET SVN_REV=Beta_1

REM Get the current date and time
FOR /F "tokens=2 delims==" %%I IN ('wmic os get localdatetime /value') DO SET DATETIME=%%I
SET SVN_DATE=%DATETIME:~0,4%-%DATETIME:~4,2%-%DATETIME:~6,2% %DATETIME:~8,2%:%DATETIME:~10,2%

REM Create the svn_rev.h file with the defined revision and date
(
    ECHO #define SVN_REV "%SVN_REV%"
    ECHO #define SVN_DATE "%SVN_DATE%"
) > %REV_FILE%

REM Clean up variables
SET REV_FILE=
SET SVN_REV=
SET SVN_DATE=
