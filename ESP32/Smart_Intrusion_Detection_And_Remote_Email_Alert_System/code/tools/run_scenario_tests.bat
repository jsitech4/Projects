@echo off
setlocal

set "PIO=%USERPROFILE%\.platformio\penv\Scripts\platformio.exe"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%PIO%" (
  echo PlatformIO was not found at "%PIO%".
  exit /b 1
)
if not exist "%VSWHERE%" (
  echo Visual Studio Build Tools were not found.
  exit /b 1
)

for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT (
  echo Visual Studio C++ Build Tools are not installed.
  exit /b 1
)

call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

if not exist ".pio\libdeps\native\Unity\src\unity.c" (
  "%PIO%" pkg install -e native
  if errorlevel 1 exit /b 1
)

cl /nologo /EHsc /Fo.pio\ /I lib /I .pio\libdeps\native\Unity\src test\test_scenarios\test_main.cpp .pio\libdeps\native\Unity\src\unity.c /Fe:.pio\scenario-tests.exe
if errorlevel 1 exit /b 1

.pio\scenario-tests.exe
exit /b %errorlevel%
