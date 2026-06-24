@echo off
setlocal

set VS_ROOT=C:\Program Files\Microsoft Visual Studio\18\Insiders
set VCVARS="%VS_ROOT%\VC\Auxiliary\Build\vcvars64.bat"
set CMAKE="%VS_ROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set NINJA="%VS_ROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

if not exist %VCVARS% (
    echo Error: Visual Studio not found at expected path.
    echo Expected: %VS_ROOT%
    pause
    exit /b 1
)

echo Setting up MSVC environment...
call %VCVARS%

if not exist build mkdir build
cd build

echo Configuring...
%CMAKE% .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=%NINJA%
if errorlevel 1 (
    echo CMake configuration failed.
    cd ..
    pause
    exit /b 1
)

echo Building...
%CMAKE% --build . --config Release
if errorlevel 1 (
    echo Build failed.
    cd ..
    pause
    exit /b 1
)

cd ..
echo.
echo ============================================
echo  Build successful: build\decimator.exe
echo ============================================
echo.
echo Usage examples:
echo   build\decimator.exe model.stl --ratio 50
echo   build\decimator.exe model.stl --max-error 0.1 --output out.stl
echo.
pause
