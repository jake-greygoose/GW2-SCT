@echo off
setlocal

set "GW2SCT_VERSION_STRING=0.0.0.0"
echo Forcing GW2-SCT version to %GW2SCT_VERSION_STRING% for testing build.

call "%~dp0build_gw2sct.bat"
if errorlevel 1 (
  echo Forced-version build failed.
  exit /b 1
)

echo Forced-version build completed successfully.
endlocal
