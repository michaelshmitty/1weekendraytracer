@echo off
set CommonCompilerFlags=-MTd -nologo -GR- -EHa- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -FC -Z7 -EHsc
set PlatformCompilerFlags=/I ..\lib\SDL2\include
set CommonLinkerFlags=-incremental:no -opt:ref
set PlatformLinkerFlags=/LIBPATH:..\lib\SDL2\lib\x64 /SUBSYSTEM:CONSOLE SDL2.lib SDL2main.lib

REM call  "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64

cls
if not exist "build" mkdir build
pushd build
del /Q /F *.* > NUL 2> NUL
copy ..\lib\SDL2\lib\x64\SDL2.dll > NUL 2> NUL
REM BUILD PLUGIN LIBRARY
cl %CommonCompilerFlags% ..\code\rt_weekend.cpp -Fmrt_weekend.map -LD /link %CommonLinkerFlags% -PDB:rt_weekend_%random%.pdb -EXPORT:PluginUpdateAndRender
REM BUILD PLATFORM LAYER
cl %CommonCompilerFlags% %PlatformCompilerFlags% ..\code\win32_platform.cpp -Fmwin32_platform.map /link %CommonLinkerFlags% %PlatformLinkerFlags%
popd
