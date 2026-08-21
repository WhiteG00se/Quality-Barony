@echo off
"%~dp0QualityBaronyLauncher.exe" %*
set "QUALITY_BARONY_EXIT=%ERRORLEVEL%"
if not "%QUALITY_BARONY_EXIT%"=="0" pause
exit /b %QUALITY_BARONY_EXIT%
