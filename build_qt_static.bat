@echo off
REM Script to build Qt 6.10.1 statically
REM Run this once from "x64 Native Tools Command Prompt for VS 2022"

echo Building Qt 6.10.1 statically...
echo This will take 1-3 hours. Go get coffee!

cd C:\Qt\6.10.1\Src

REM Configure Qt for static build
configure -static -release -platform win32-msvc ^
  -prefix C:\Qt\6.10.1-static ^
  -nomake examples -nomake tests ^
  -skip qtwebengine -skip qt3d -skip qtquick3d -skip qtquickcontrols ^
  -skip qtmultimedia -skip qtwebview -skip qtlocation ^
  -opensource -confirm-license

REM Build Qt
cmake --build . --parallel
cmake --install .

echo.
echo Qt static build complete!
echo Installed to: C:\Qt\6.10.1-static
pause
