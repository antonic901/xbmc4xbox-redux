@echo off
setlocal

rem Resolve the repository root so this script can run from any directory.
pushd "%~dp0..\.."
if errorlevel 1 exit /b 1

set "output_dir=xbmc\interfaces\python\generated"
if not exist "%output_dir%\" mkdir "%output_dir%"
if not exist "%output_dir%\" goto failed

for %%I in (xbmc\interfaces\swig\*.i) do (
  echo Generating %%~nxI.cpp

  swig -w401 -c++ -o "%output_dir%\%%~nxI.xml" -xml -Ixbmc "%%I"
  if errorlevel 1 goto failed

  java -cp "tools\codegenerator\groovy\*;tools\codegenerator;xbmc\interfaces\python" groovy.ui.GroovyMain tools\codegenerator\Generator.groovy "%output_dir%\%%~nxI.xml" xbmc\interfaces\python\PythonSwig.cpp.template "%output_dir%\%%~nxI.cpp"
  if errorlevel 1 goto failed
)

popd
endlocal
exit /b 0

:failed
echo Binding generation failed. 1>&2
popd
endlocal
exit /b 1
