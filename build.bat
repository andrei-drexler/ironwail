@echo off
setlocal enableextensions

echo Build...
git pull


rem --- Paths ---
set "SRC_SHADERS=C:\ironwail\Quake\shaders"
set "DST_DIR=C:\Program Files (x86)\Steam\steamapps\common\Quake\rerelease"
set "DST_SHADERS=%DST_DIR%\id1\shaders"

set "SRC_EXE=C:\ironwail\Windows\VisualStudio\Build-ironwail\bin\x64\Release\ironwail.exe"
set "DST_EXE=%DST_DIR%\ironwail.exe"

rem --- Ensure destination folders exist ---
if not exist "%DST_DIR%\id1" mkdir "%DST_DIR%\id1"
if not exist "%DST_SHADERS%" mkdir "%DST_SHADERS%"

rem --- Copy shaders directory (use robocopy for folders) ---
robocopy "%SRC_SHADERS%" "%DST_SHADERS%" *.* /E /R:1 /W:1 >nul
set "RC=%ERRORLEVEL%"
if %RC% GEQ 8 (
    echo [ERROR] Copying shaders failed with robocopy code %RC%.
    exit /b %RC%
)

rem --- Copy executable ---
copy /Y "%SRC_EXE%" "%DST_EXE%" >nul
if errorlevel 1 (
    echo [ERROR] Copying ironwail.exe failed.
    exit /b 1
)

echo Done.
exit /b 0
