@echo off

IF NOT EXIST "%ProgramFiles%\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat" GOTO Vc7_Check2
CALL "%ProgramFiles%\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"
GOTO VC_DONE

:Vc7_Check2
IF NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat" GOTO Vc9_Check
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"
GOTO VC_DONE

:Vc9_Check
IF NOT EXIST "%ProgramFiles%\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat" GOTO Vc9_Check2
CALL "%ProgramFiles%\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat"
GOTO VC_DONE

:Vc9_Check2
IF NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat" GOTO NO_VC
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat"
GOTO VC_DONE

:NO_VC
ECHO "NOTE: Microsoft Visual Studio installation was NOT found!"
PAUSE

:VC_DONE
CALL msys\1.0\msys.bat 