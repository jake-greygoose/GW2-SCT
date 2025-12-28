@echo off
setlocal

set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"
set "BUILD_DIR=%REPO_ROOT%\out\build"
set "DIST_DIR=%REPO_ROOT%\dist"

if not defined CMAKE (
  set "CMAKE=cmake"
)
if defined GW2SCT_CMAKE (
  set "CMAKE=%GW2SCT_CMAKE%"
)
if not defined VCPKG (
  set "VCPKG=%USERPROFILE%\vcpkg"
)

set "CMAKE_TOOLCHAIN_ARGS="
if exist "%VCPKG%\scripts\buildsystems\vcpkg.cmake" (
  echo [info] Using vcpkg toolchain from %VCPKG%
  set "VCPKG_TOOLCHAIN_FILE=%VCPKG%\scripts\buildsystems\vcpkg.cmake"
  set "CMAKE_TOOLCHAIN_ARGS=-DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET=x64-windows-static"
) else (
  echo [info] vcpkg toolchain not found at %VCPKG%\scripts\buildsystems\vcpkg.cmake - continuing without it.
)

echo Cleaning previous build directory...
if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"

echo Configuring Release build with CMake...
"%CMAKE%" -S "%REPO_ROOT%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 %CMAKE_TOOLCHAIN_ARGS%
if errorlevel 1 goto :error

echo Building Release configuration...
"%CMAKE%" --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 goto :error

set "BIN_DIR=%BUILD_DIR%\Release"
set "DLL_PATH=%BIN_DIR%\gw2-sct.dll"
if not exist "%DLL_PATH%" (
  set "BIN_DIR=%BUILD_DIR%"
  set "DLL_PATH=%BIN_DIR%\gw2-sct.dll"
)
if not exist "%DLL_PATH%" (
  echo Build artifact not found after compilation.
  goto :error
)

set "PDB_PATH=%BIN_DIR%\gw2-sct.pdb"
if not exist "%PDB_PATH%" (
  echo PDB not found next to "%DLL_PATH%".
  goto :error
)

if exist "%DIST_DIR%" rd /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%" >nul
copy /y "%DLL_PATH%" "%DIST_DIR%" >nul
copy /y "%PDB_PATH%" "%DIST_DIR%" >nul

echo Build completed. Artifacts copied to "%DIST_DIR%".
goto :eof

:error
echo Build failed. See messages above for details.
exit /b 1
