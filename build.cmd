@echo off
cd /d "%~dp0"
set PYTHONDONTWRITEBYTECODE=1
"%~dp0tools\python\python3.exe" "%~dp0build.py" %*
exit /b %errorlevel%
