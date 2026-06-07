@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Compiling resources...
rc src\resources.rc

echo Compiling main.cpp with icon...
cl /std:c++20 /EHsc /await:strict /DUNICODE /D_UNICODE src\main.cpp src\resources.res /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\cppwinrt" /link /SUBSYSTEM:WINDOWS /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64" /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64" windowsapp.lib user32.lib shell32.lib /out:daydream.exe

if %ERRORLEVEL% equ 0 (
    echo Success: daydream.exe created with icon.
) else (
    echo Error: Compilation failed.
)
