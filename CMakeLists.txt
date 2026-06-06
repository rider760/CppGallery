cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)

project(CppGallery LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(CPPGALLERY_FETCH_DEPS "Download GLFW / ImGui / stb_image.h when missing." ON)
option(CPPGALLERY_ENABLE_FFMPEG "Enable FFmpeg video support when development files are available." ON)
option(CPPGALLERY_EMBED_FFMPEG_TOOLS "Embed local ffmpeg.exe + ffprobe.exe into the Windows executable." OFF)
set(CPPGALLERY_FFMPEG_ROOT "" CACHE PATH "Path to an FFmpeg development package root containing include/ and lib/.")

include(FetchContent)

find_package(OpenGL REQUIRED)

set(CPPGALLERY_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

function(cppgallery_resolve_glfw out_target)
    if(TARGET glfw)
        set(${out_target} glfw PARENT_SCOPE)
        return()
    endif()

    if(TARGET glfw3::glfw)
        set(${out_target} glfw3::glfw PARENT_SCOPE)
        return()
    endif()

    find_package(glfw3 CONFIG QUIET)
    if(TARGET glfw)
        set(${out_target} glfw PARENT_SCOPE)
        return()
    endif()
    if(TARGET glfw3::glfw)
        set(${out_target} glfw3::glfw PARENT_SCOPE)
        return()
    endif()

    set(local_glfw_dir "${CPPGALLERY_THIRD_PARTY_DIR}/glfw")
    if(EXISTS "${local_glfw_dir}/CMakeLists.txt")
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        add_subdirectory("${local_glfw_dir}" "${CMAKE_BINARY_DIR}/glfw-local")
    elseif(CPPGALLERY_FETCH_DEPS)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(glfw)
    else()
        message(FATAL_ERROR "GLFW not found and CPPGALLERY_FETCH_DEPS=OFF.")
    endif()

    if(TARGET glfw)
        set(${out_target} glfw PARENT_SCOPE)
    elseif(TARGET glfw3::glfw)
        set(${out_target} glfw3::glfw PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Failed to resolve a GLFW target.")
    endif()
endfunction()

function(cppgallery_resolve_imgui out_target)
    if(TARGET imgui)
        set(${out_target} imgui PARENT_SCOPE)
        return()
    endif()

    set(local_imgui_dir "${CPPGALLERY_THIRD_PARTY_DIR}/imgui")
    if(EXISTS "${local_imgui_dir}/imgui.cpp")
        set(imgui_source_dir "${local_imgui_dir}")
    elseif(CPPGALLERY_FETCH_DEPS)
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.91.8
            GIT_SHALLOW TRUE
        )
        FetchContent_GetProperties(imgui)
        if(NOT imgui_POPULATED)
            FetchContent_Populate(imgui)
        endif()
        set(imgui_source_dir "${imgui_SOURCE_DIR}")
    else()
        message(FATAL_ERROR "Dear ImGui not found and CPPGALLERY_FETCH_DEPS=OFF.")
    endif()

    if(NOT EXISTS "${imgui_source_dir}/backends/imgui_impl_glfw.cpp")
        message(FATAL_ERROR "Resolved ImGui source tree is incomplete: ${imgui_source_dir}")
    endif()

    add_library(imgui STATIC
        "${imgui_source_dir}/imgui.cpp"
        "${imgui_source_dir}/imgui_demo.cpp"
        "${imgui_source_dir}/imgui_draw.cpp"
        "${imgui_source_dir}/imgui_tables.cpp"
        "${imgui_source_dir}/imgui_widgets.cpp"
        "${imgui_source_dir}/backends/imgui_impl_glfw.cpp"
        "${imgui_source_dir}/backends/imgui_impl_opengl3.cpp"
    )

    target_include_directories(imgui
        PUBLIC
            "${imgui_source_dir}"
            "${imgui_source_dir}/backends"
    )

    target_compile_features(imgui PUBLIC cxx_std_17)
    set(${out_target} imgui PARENT_SCOPE)
endfunction()

cppgallery_resolve_glfw(CPPGALLERY_GLFW_TARGET)
cppgallery_resolve_imgui(CPPGALLERY_IMGUI_TARGET)

set(CPPGALLERY_STB_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/stb_image.h")
if(NOT EXISTS "${CPPGALLERY_STB_HEADER}")
    message(FATAL_ERROR "Missing stb_image.h. Place stb_image.h in ${CMAKE_CURRENT_SOURCE_DIR}.")
endif()

target_link_libraries(${CPPGALLERY_IMGUI_TARGET}
    PUBLIC
        ${CPPGALLERY_GLFW_TARGET}
        OpenGL::GL
)

set(CPPGALLERY_WINDOWS_RESOURCE)
set(CPPGALLERY_HAS_EMBEDDED_FFMPEG_TOOLS OFF)
if(WIN32)
    set(CPPGALLERY_WINDOWS_RESOURCE "${CMAKE_CURRENT_BINARY_DIR}/cppgallery_windows_resources.rc")
    set(CPPGALLERY_RESOURCE_LINES "#include <winres.h>\n#define IDI_APP_ICON 101\n")

    set(CPPGALLERY_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/17807185225986.ico")
    if(EXISTS "${CPPGALLERY_APP_ICON}")
        file(TO_CMAKE_PATH "${CPPGALLERY_APP_ICON}" CPPGALLERY_APP_ICON_RC)
        string(APPEND CPPGALLERY_RESOURCE_LINES "IDI_APP_ICON ICON \"${CPPGALLERY_APP_ICON_RC}\"\n")
        message(STATUS "Embedding application icon from ${CPPGALLERY_APP_ICON}")
    else()
        message(WARNING "Skipping application icon embedding: ${CPPGALLERY_APP_ICON} not found.")
    endif()

    if(CPPGALLERY_EMBED_FFMPEG_TOOLS)
        set(CPPGALLERY_LOCAL_FFMPEG_EXE "${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg.exe")
        set(CPPGALLERY_LOCAL_FFPROBE_EXE "${CMAKE_CURRENT_SOURCE_DIR}/ffprobe.exe")
        if(EXISTS "${CPPGALLERY_LOCAL_FFMPEG_EXE}" AND EXISTS "${CPPGALLERY_LOCAL_FFPROBE_EXE}")
            file(TO_CMAKE_PATH "${CPPGALLERY_LOCAL_FFMPEG_EXE}" CPPGALLERY_LOCAL_FFMPEG_EXE_RC)
            file(TO_CMAKE_PATH "${CPPGALLERY_LOCAL_FFPROBE_EXE}" CPPGALLERY_LOCAL_FFPROBE_EXE_RC)
            string(APPEND CPPGALLERY_RESOURCE_LINES
                "41001 RCDATA \"${CPPGALLERY_LOCAL_FFMPEG_EXE_RC}\"\n"
                "41002 RCDATA \"${CPPGALLERY_LOCAL_FFPROBE_EXE_RC}\"\n")
            set(CPPGALLERY_HAS_EMBEDDED_FFMPEG_TOOLS ON)
            message(STATUS "Embedding ffmpeg.exe and ffprobe.exe into CppGallery.exe")
        else()
            message(STATUS "Skipping FFmpeg tool embedding: ffmpeg.exe and/or ffprobe.exe not found in source dir.")
        endif()
    endif()

    file(WRITE "${CPPGALLERY_WINDOWS_RESOURCE}" "${CPPGALLERY_RESOURCE_LINES}")
endif()

add_executable(CppGallery main.cpp ${CPPGALLERY_WINDOWS_RESOURCE})

target_include_directories(CppGallery
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(CPPGALLERY_HAS_EMBEDDED_FFMPEG_TOOLS)
    target_compile_definitions(CppGallery PRIVATE CPPGALLERY_HAS_EMBEDDED_FFMPEG_TOOLS=1)
endif()

if(WIN32)
    set_target_properties(CppGallery PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(CppGallery PRIVATE /ENTRY:mainCRTStartup)
    endif()
    target_link_libraries(CppGallery PRIVATE dwmapi)
endif()

target_link_libraries(CppGallery
    PRIVATE
        ${CPPGALLERY_IMGUI_TARGET}
        ${CPPGALLERY_GLFW_TARGET}
        OpenGL::GL
)

if(CPPGALLERY_ENABLE_FFMPEG)
    set(CPPGALLERY_FFMPEG_SEARCH_ROOTS)
    if(CPPGALLERY_FFMPEG_ROOT)
        list(APPEND CPPGALLERY_FFMPEG_SEARCH_ROOTS "${CPPGALLERY_FFMPEG_ROOT}")
    endif()
    if(DEFINED ENV{FFMPEG_ROOT} AND NOT "$ENV{FFMPEG_ROOT}" STREQUAL "")
        list(APPEND CPPGALLERY_FFMPEG_SEARCH_ROOTS "$ENV{FFMPEG_ROOT}")
    endif()
    list(APPEND CPPGALLERY_FFMPEG_SEARCH_ROOTS "${CPPGALLERY_THIRD_PARTY_DIR}/ffmpeg")

    find_path(CPPGALLERY_FFMPEG_INCLUDE_DIR
        NAMES libavcodec/avcodec.h
        PATHS ${CPPGALLERY_FFMPEG_SEARCH_ROOTS}
        PATH_SUFFIXES include
    )

    set(CPPGALLERY_FFMPEG_LIBRARIES)
    set(CPPGALLERY_FFMPEG_FOUND TRUE)
    foreach(component IN ITEMS avcodec avformat avutil swscale)
        string(TOUPPER "${component}" component_upper)
        find_library(CPPGALLERY_${component_upper}_LIBRARY
            NAMES ${component}
            PATHS ${CPPGALLERY_FFMPEG_SEARCH_ROOTS}
            PATH_SUFFIXES lib lib64 lib/x64
        )
        if(CPPGALLERY_${component_upper}_LIBRARY)
            list(APPEND CPPGALLERY_FFMPEG_LIBRARIES "${CPPGALLERY_${component_upper}_LIBRARY}")
        else()
            set(CPPGALLERY_FFMPEG_FOUND FALSE)
        endif()
    endforeach()

    if(CPPGALLERY_FFMPEG_INCLUDE_DIR AND CPPGALLERY_FFMPEG_FOUND)
        add_library(cppgallery_ffmpeg INTERFACE)
        target_include_directories(cppgallery_ffmpeg INTERFACE "${CPPGALLERY_FFMPEG_INCLUDE_DIR}")
        target_link_libraries(cppgallery_ffmpeg INTERFACE ${CPPGALLERY_FFMPEG_LIBRARIES})
        target_link_libraries(CppGallery PRIVATE cppgallery_ffmpeg)
        target_compile_definitions(CppGallery PRIVATE CPPGALLERY_HAS_FFMPEG=1)
        message(STATUS "FFmpeg support enabled from ${CPPGALLERY_FFMPEG_INCLUDE_DIR}")
    else()
        message(STATUS "FFmpeg development files not found. Runtime video playback uses external ffmpeg.exe + ffprobe.exe when present.")
    endif()
endif()

if(MSVC)
    target_compile_options(CppGallery PRIVATE /W4 /permissive- /utf-8)
    target_compile_definitions(CppGallery PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(imgui PRIVATE /W3 /utf-8)
else()
    target_compile_options(CppGallery PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(APPLE)
    target_compile_definitions(CppGallery PRIVATE GL_SILENCE_DEPRECATION)
endif()
