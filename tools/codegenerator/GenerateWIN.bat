@ECHO OFF

SET cur_dir=%CD%

SET swigFolder=%cur_dir%\..\..\xbmc\interfaces\swig
SET pythonFolder=%cur_dir%\..\..\xbmc\interfaces\python

for %%F in (%swigFolder%\*.i) do (
    ECHO Generating from: %%~nxF
    call GenerateSWIGBindings.bat %pythonFolder% %%~nF
)
