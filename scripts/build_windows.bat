@echo off
rem Usage: scripts\build_windows.bat [debug|release]   (default: debug)
rem
rem Prefers clang-cl, then clang, then MSVC's cl.exe. cl.exe and clang-cl both
rem need the environment a Developer Command Prompt sets up.
rem
rem Only src\main.c is compiled. Everything else reaches the compiler through
rem src\unity_build.c.

setlocal

set "script_dir=%~dp0"
set "root_dir=%script_dir%.."
set "build_dir=%root_dir%\build"
set "src=%root_dir%\src\main.c"

set "mode=%~1"
if "%mode%"=="" set "mode=debug"
if /i not "%mode%"=="debug" if /i not "%mode%"=="release" (
    echo usage: %~nx0 [debug^|release] 1>&2
    exit /b 1
)

if not exist "%build_dir%" mkdir "%build_dir%"

where /q clang-cl && goto :clang_cl
where /q clang && goto :clang
where /q cl && goto :msvc

echo error: no compiler found. Need clang-cl, clang, or cl.exe on PATH. 1>&2
echo        For cl.exe, run this from a Developer Command Prompt. 1>&2
exit /b 1

rem ---------------------------------------------------------------- clang-cl
:clang_cl
if /i "%mode%"=="debug" (
    set "flags=/W4 /Od /Zi /DBUILD_DEBUG=1"
) else (
    set "flags=/W4 /O2 /DBUILD_DEBUG=0 /DNDEBUG"
)
echo clang-cl %flags% "%src%"
clang-cl /nologo /std:c11 /D_CRT_SECURE_NO_WARNINGS %flags% "%src%" ^
    /Fe:"%build_dir%\csh.exe" /Fo:"%build_dir%\\" /Fd:"%build_dir%\\"
exit /b %errorlevel%

rem ------------------------------------------------------------------- clang
:clang
if /i "%mode%"=="debug" (
    set "flags=-g -O0 -DBUILD_DEBUG=1"
) else (
    set "flags=-O2 -DBUILD_DEBUG=0 -DNDEBUG"
)
echo clang %flags% "%src%"
clang -std=gnu11 -Wall -Wextra %flags% "%src%" -o "%build_dir%\csh.exe"
exit /b %errorlevel%

rem ------------------------------------------------------------------ cl.exe
:msvc
if /i "%mode%"=="debug" (
    set "flags=/W4 /Od /Zi /DBUILD_DEBUG=1"
) else (
    set "flags=/W4 /O2 /DBUILD_DEBUG=0 /DNDEBUG"
)
echo cl %flags% "%src%"
rem /std:c11 needs MSVC 2019 16.8 or newer.
cl /nologo /std:c11 /D_CRT_SECURE_NO_WARNINGS %flags% "%src%" ^
    /Fe:"%build_dir%\csh.exe" /Fo:"%build_dir%\\" /Fd:"%build_dir%\\"
exit /b %errorlevel%
