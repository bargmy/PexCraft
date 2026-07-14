cmake_minimum_required(VERSION 3.22.1)
project(pexcraft_android_tv C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

get_filename_component(ANDROID_TV_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
get_filename_component(REPO_ROOT "${ANDROID_TV_ROOT}/.." ABSOLUTE)

set(SDL2_SOURCE_ROOT "${ANDROID_TV_ROOT}/third_party/SDL2-src")
set(SDL2_INCLUDE_ROOT "${ANDROID_TV_ROOT}/third_party/SDL2/include")

if(NOT EXISTS "${SDL2_SOURCE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "SDL2 source missing. Run android_tv/prepare_sdl2.sh before building.")
endif()
if(NOT EXISTS "${SDL2_INCLUDE_ROOT}/SDL2/SDL.h")
    message(FATAL_ERROR "SDL2 headers missing. Run android_tv/prepare_sdl2.sh before building.")
endif()

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory("${SDL2_SOURCE_ROOT}" "${CMAKE_BINARY_DIR}/SDL2")

if(TARGET SDL2::SDL2)
    set(PEX_SDL2_TARGET SDL2::SDL2)
elseif(TARGET SDL2)
    set(PEX_SDL2_TARGET SDL2)
else()
    message(FATAL_ERROR "SDL2 CMake target was not created.")
endif()

add_library(main SHARED "${REPO_ROOT}/src/main_android_tv.c")

include("${REPO_ROOT}/cmake/PexBuildInfo.cmake")
pex_enable_build_info(main "${REPO_ROOT}")

target_compile_definitions(main PRIVATE
    PEX_PLATFORM_ANDROID_TV=1
    PEX_PLATFORM_SDL2=1
)

target_include_directories(main PRIVATE
    "${REPO_ROOT}/src"
    "${SDL2_INCLUDE_ROOT}"
)

target_compile_options(main PRIVATE
    -Wall
    -Wextra
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-parameter
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:RelWithDebInfo>:-O3>
)

target_link_libraries(main
    ${PEX_SDL2_TARGET}
    GLESv2
    EGL
    android
    log
    z
    m
)
