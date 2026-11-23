@echo off
setlocal

git -C "%~dp0." submodule update --init --recursive
if errorlevel 1 exit /b %errorlevel%

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0BuildDependencies.ps1"
exit /b %errorlevel%
