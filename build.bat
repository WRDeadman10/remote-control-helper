@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo  Building LAN Camera Remote Control Helper (x64)
echo ===================================================

set HELPER_ROOT=%~dp0
set BUILD_DIR=%HELPER_ROOT%build

if not exist "%BUILD_DIR%" (
    cmake -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
)

echo Compiling remote-control-helper.exe (Release)...
cmake --build "%BUILD_DIR%" --config Release
if %ERRORLEVEL% neq 0 (
    echo Error compiling remote-control-helper.exe!
    exit /b %ERRORLEVEL%
)

echo Building the single-file Inno Setup installer...
set "ISCC="
if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"

if not defined ISCC (
    echo [WARN] Inno Setup 6 was not found. Skipping installer generation.
    exit /b 0
)

set APP_VERSION=1.0.0
if exist "%HELPER_ROOT%..\package.json" (
    for /f "usebackq delims=" %%V in (`powershell -NoProfile -Command "(Get-Content -Raw '%HELPER_ROOT%..\package.json' | ConvertFrom-Json).version"`) do set APP_VERSION=%%V
)
if defined LANCAM_VERSION set APP_VERSION=%LANCAM_VERSION%

set OUT_DIR=%BUILD_DIR%\installer
if exist "%HELPER_ROOT%..\package.json" set OUT_DIR=%HELPER_ROOT%..\dist
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

"%ISCC%" /Qp /DAppVersion=%APP_VERSION% /DBuildDir="%BUILD_DIR%\Release" /DOutDir="%OUT_DIR%" "%HELPER_ROOT%installer\control-helper.iss"
if %ERRORLEVEL% neq 0 (
    echo Error building the Inno Setup installer!
    exit /b %ERRORLEVEL%
)

echo.
echo Build successful! Outputs:
echo   - %BUILD_DIR%\Release\remote-control-helper.exe
echo   - %OUT_DIR%\control-helper-setup.exe
echo.
