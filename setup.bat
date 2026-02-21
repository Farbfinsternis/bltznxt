@echo off
setlocal
cd /d %~dp0

:: Try to find python in tools/python first (portable)
set PY=tools\python\python.exe
if not exist "%PY%" set PY=python

echo Running setup script...
"%PY%" scripts/setup.py
if %ERRORLEVEL% neq 0 (
    echo.
    echo Setup failed. Please ensure Python is installed and in your PATH,
    echo or that you have the portable Python package in tools/python.
    pause
    exit /b 1
)

echo Setup successful!
pause
