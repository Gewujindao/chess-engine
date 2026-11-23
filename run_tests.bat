@echo off
rem Windows entry point: load the MSVC x64 environment, then run the full
rem test pipeline (build, engine tests, python-chess cross-check).
setlocal
cd /d "%~dp0"

for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul

".venv\Scripts\python.exe" tools\run_tests.py
exit /b %errorlevel%
