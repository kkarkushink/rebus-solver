@echo off
rem Build the solver and benchmark with MSVC (Visual Studio 2022).
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist bin mkdir bin
cl /nologo /O2 /std:c11 /utf-8 /Fo:bin\ /Fe:bin\solver.exe src\main.c src\puzzle.c
cl /nologo /O2 /std:c11 /utf-8 /Fo:bin\ /Fe:bin\bench.exe  src\bench.c src\puzzle.c
cl /nologo /O2 /std:c11 /utf-8 /Fo:bin\ /Fe:bin\tests.exe  src\test.c src\puzzle.c
echo.
echo Done: bin\solver.exe "SEND + MORE = MONEY"
echo       bin\bench.exe
echo       bin\tests.exe
echo.
bin\tests.exe
if errorlevel 1 exit /b 1
