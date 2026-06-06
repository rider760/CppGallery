@echo off
echo Loading MSVC Environment...
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
if not exist build_nmake\CMakeCache.txt (
    echo Configuring NMake build...
    cmake -S . -B build_nmake -G "NMake Makefiles" -DCPPGALLERY_FETCH_DEPS=ON -DFETCHCONTENT_SOURCE_DIR_GLFW="%CD%\build\_deps\glfw-src" -DFETCHCONTENT_SOURCE_DIR_IMGUI="%CD%\build\_deps\imgui-src"
    if %ERRORLEVEL% NEQ 0 (
        echo CONFIG_FAILED_MARKER
        exit /b %ERRORLEVEL%
    )
)
echo Compiling...
cmake --build build_nmake
if %ERRORLEVEL% NEQ 0 (
    echo BUILD_FAILED_MARKER
    exit /b %ERRORLEVEL%
)
if not exist build\Release mkdir build\Release
copy /Y .\build_nmake\CppGallery.exe .\build\Release\CppGallery.exe >nul
echo BUILD SUCCESS. Launching App...
".\build\Release\CppGallery.exe" ".\build\Release"
