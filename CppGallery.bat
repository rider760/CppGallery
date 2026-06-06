@echo off
if exist .\build\Release\CppGallery.exe (
    start "" .\build\Release\CppGallery.exe .\build\Release
    exit /b 0
)
if exist .\build_nmake\CppGallery.exe (
    start "" .\build_nmake\CppGallery.exe .\build\Release
    exit /b 0
)
echo CppGallery executable not found. Run auto_build.bat first.
exit /b 1
