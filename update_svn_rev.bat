@echo off
setlocal EnableExtensions DisableDelayedExpansion

SET "REV_TEMPLATE=%~dp0xbmc\svn_rev.tmpl"
SET "REV_FILE=%~dp0xbmc\svn_rev.h"

REM Local development build version.
SET "APP_VERSION_MAJOR=0"
SET "APP_VERSION_MINOR=0"
SET "APP_VERSION_PATCH=0"
SET "APP_VERSION_CODE=%APP_VERSION_MAJOR%.%APP_VERSION_MINOR%.%APP_VERSION_PATCH%"
SET "APP_CHANNEL=local"
SET "APP_SCMID=unavailable"

IF NOT EXIST "%REV_TEMPLATE%" (
    ECHO Revision template not found: "%REV_TEMPLATE%" 1>&2
    EXIT /B 1
)

REM Use WMIC on XP Professional for a locale-independent timestamp.
REM Fall back to the local date format if WMIC is not available.
SET "BUILD_DATETIME="
FOR /F "tokens=2 delims==" %%I IN ('wmic os get localdatetime /value 2^>nul') DO SET "BUILD_DATETIME=%%I"
IF DEFINED BUILD_DATETIME (
    SET "APP_BUILD_DATE=%BUILD_DATETIME:~0,4%-%BUILD_DATETIME:~4,2%-%BUILD_DATETIME:~6,2% %BUILD_DATETIME:~8,2%:%BUILD_DATETIME:~10,2%"
) ELSE (
    SET "APP_BUILD_DATE=%DATE% %TIME:~0,5%"
)

REM Number every line so FOR /F preserves blank lines and the template header.
REM Enable delayed expansion only after reading each line to preserve ! characters.
VER >NUL
> "%REV_FILE%" (
    FOR /F "usebackq delims=" %%L IN (`findstr /n "^" "%REV_TEMPLATE%"`) DO (
        SET "LINE=%%L"
        SETLOCAL EnableDelayedExpansion
        SET "LINE=!LINE:*:=!"
        REM An empty LINE is undefined; do not apply substitutions to it.
        IF DEFINED LINE (
            SET "LINE=!LINE:$APP_CHANNEL$=%APP_CHANNEL%!"
            SET "LINE=!LINE:$APP_VERSION_MAJOR$=%APP_VERSION_MAJOR%!"
            SET "LINE=!LINE:$APP_VERSION_MINOR$=%APP_VERSION_MINOR%!"
            SET "LINE=!LINE:$APP_VERSION_PATCH$=%APP_VERSION_PATCH%!"
            SET "LINE=!LINE:$APP_VERSION_CODE$=%APP_VERSION_CODE%!"
            SET "LINE=!LINE:$WCREV$=%APP_SCMID%!"
            SET "LINE=!LINE:$WCDATE$=%APP_BUILD_DATE%!"
        )
        ECHO(!LINE!
        ENDLOCAL
    )
)
IF ERRORLEVEL 1 EXIT /B 1
EXIT /B 0
