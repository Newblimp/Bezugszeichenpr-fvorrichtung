@echo off
REM Script to build Qt 6.10.1 statically
REM IMPORTANT: Run this from "x64 Native Tools Command Prompt for VS 2022"

echo ========================================
echo Building Qt 6.10.1 statically
echo This will take 1-3 hours. Go get coffee!
echo ========================================
echo.

REM Check if source directory exists
if not exist "C:\Qt\6.10.1\Src" (
    echo ERROR: Qt source not found at C:\Qt\6.10.1\Src
    echo Please download and extract Qt source first.
    pause
    exit /b 1
)

cd /d C:\Qt\6.10.1\Src

echo Step 1: Configuring Qt...
echo.

REM Configure Qt for static build
call configure.bat -static -release -platform win32-msvc ^
  -prefix C:\Qt\6.10.1-static ^
  -nomake examples -nomake tests ^
  -skip qtwebengine -skip qt3d -skip qtquick3d -skip qtquickcontrols ^
  -skip qtmultimedia -skip qtwebview -skip qtlocation ^
  -opensource -confirm-license

if errorlevel 1 (
    echo.
    echo ERROR: Qt configuration failed!
    pause
    exit /b 1
)

echo.
echo Step 2: Building Qt (this takes 1-3 hours)...
echo.

REM Build Qt
cmake --build . --parallel --config Release

if errorlevel 1 (
    echo.
    echo ERROR: Qt build failed!
    pause
    exit /b 1
)

echo.
echo Step 3: Installing Qt to C:\Qt\6.10.1-static...
echo.

cmake --install . --config Release

if errorlevel 1 (
    echo.
    echo ERROR: Qt installation failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo SUCCESS: Qt static build complete!
echo Installed to: C:\Qt\6.10.1-static
echo ========================================
pause
