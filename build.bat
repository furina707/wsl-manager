@echo off
setlocal enabledelayedexpansion

echo Checking for Compiler

:: 设置路径
set "SRC_DIR=src"
set "BIN_DIR=bin"

if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"

REM 检查 MSYS2 GCC
set "GCC_PATH=C:\msys64\mingw64\bin"
if exist "%GCC_PATH%\gcc.exe" (
    echo Found GCC in MSYS2.
    set "PATH=%GCC_PATH%;%PATH%"
    
    echo Compiling resources
    windres "%SRC_DIR%\resource.rc" -o "%SRC_DIR%\resource.o"
    
    echo Compiling wsl_manager.c (CLI)
    gcc "%SRC_DIR%\wsl_manager.c" "%SRC_DIR%\wsl_core.c" -o "%BIN_DIR%\wsl_manager.exe" -municode -Wall
    
    echo Compiling wsl_gui.c (GUI)
    gcc "%SRC_DIR%\wsl_gui.c" "%SRC_DIR%\wsl_core.c" "%SRC_DIR%\resource.o" -o "%BIN_DIR%\wsl_gui.exe" -municode -mwindows -luser32 -lgdi32 -lcomctl32 -lshell32 -lole32
    
    del "%SRC_DIR%\resource.o"
    
    if !errorlevel! equ 0 (
        goto success
    ) else (
        goto error
    )
)

REM 如果没有 GCC，尝试找 MSVC
echo GCC not found, checking for Visual Studio
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"

if exist "%VCVARS%" (
    echo Found Visual Studio.
    call "%VCVARS%"
    
    echo Compiling resources
    rc /fo "%SRC_DIR%\resource.res" "%SRC_DIR%\resource.rc"
    
    echo Compiling wsl_manager.c (CLI)
    cl "%SRC_DIR%\wsl_manager.c" "%SRC_DIR%\wsl_core.c" /Fe:"%BIN_DIR%\wsl_manager.exe" /utf-8
    
    echo Compiling wsl_gui.c (GUI)
    cl "%SRC_DIR%\wsl_gui.c" "%SRC_DIR%\wsl_core.c" "%SRC_DIR%\resource.res" /Fe:"%BIN_DIR%\wsl_gui.exe" /utf-8 /link /subsystem:windows /user32.lib gdi32.lib comctl32.lib shell32.lib
    
    del "%SRC_DIR%\resource.res"
    del "%BIN_DIR%\wsl_manager.obj"
    del "%BIN_DIR%\wsl_gui.obj"
    del "%BIN_DIR%\wsl_core.obj"
    
    if !errorlevel! equ 0 (
        goto success
    ) else (
        goto error
    )
)

echo Error: No suitable compiler (GCC or MSVC) found.
pause
exit /b 1

:success
echo.
echo ========================================
echo SUCCESS: wsl_manager.exe and wsl_gui.exe created in %BIN_DIR%!
echo ========================================
echo.
echo You can now run:
echo   CLI: .\%BIN_DIR%\wsl_manager.exe
echo   GUI: .\%BIN_DIR%\wsl_gui.exe
pause
exit /b 0

:error
echo.
echo ========================================
echo ERROR: Compilation failed.
echo ========================================
pause
exit /b 1
