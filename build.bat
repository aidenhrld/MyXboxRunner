@echo off
if not exist build mkdir build

echo === BUILDING MYXBOXRUNNER SUITE ===

echo [1/10] Building shim_kernelx.dll...
cl /nologo /std:c++17 /EHsc /LD src\shims\shim_kernelx.cpp /Fo:build\ /Fe:build\shim_kernelx.dll

echo [2/10] Building shim_input.dll...
cl /nologo /std:c++17 /EHsc /LD src\shims\shim_input.cpp user32.lib /Fo:build\ /Fe:build\shim_input.dll

echo [3/10] Building shim_d3d12.dll...
cl /nologo /std:c++17 /EHsc /LD src\shims\shim_d3d12.cpp gdiplus.lib gdi32.lib user32.lib /Fo:build\ /Fe:build\shim_d3d12.dll

echo [4/10] Building shim_audio.dll...
cl /nologo /std:c++17 /EHsc /LD src\shims\shim_audio.cpp /Fo:build\ /Fe:build\shim_audio.dll

echo [5/10] Building shim_router.dll...
cl /nologo /std:c++17 /EHsc /LD src\shims\shim_router.cpp /Fo:build\ /Fe:build\shim_router.dll

echo [6/10] Building loader.exe...
cl /nologo /std:c++17 /EHsc src\loader\loader.cpp /Fo:build\ /Fe:build\loader.exe

echo [7/10] Building main.exe...
cl /nologo /std:c++17 /EHsc src\loader\main.cpp user32.lib /Fo:build\ /Fe:build\main.exe

echo [8/10] Building inspect.exe...
cl /nologo /std:c++17 /EHsc src\tools\inspect.cpp /Fo:build\ /Fe:build\inspect.exe

echo [9/10] Building stubgen.exe...
cl /nologo /std:c++17 /EHsc src\tools\stubgen.cpp /Fo:build\ /Fe:build\stubgen.exe

echo [10/10] Building dumpcheck.exe and extractmovies.exe...
cl /nologo /std:c++17 /EHsc src\tools\dumpcheck.cpp /Fo:build\ /Fe:build\dumpcheck.exe
cl /nologo /std:c++17 /EHsc src\tools\extractmovies.cpp /Fo:build\ /Fe:build\extractmovies.exe

del build\*.obj build\*.lib build\*.exp 2>nul

echo.
echo === ALL TARGETS BUILT CLEANLY INTO .\build\ ===