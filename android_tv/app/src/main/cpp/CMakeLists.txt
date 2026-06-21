cmake_minimum_required(VERSION 3.22.1)
project(pexcraft_android_tv C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

get_filename_component(ANDROID_TV_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
get_filename_component(REPO_ROOT "${ANDROID_TV_ROOT}/.." ABSOLUTE)

set(SDL2_INCLUDE_ROOT "${ANDROID_TV_ROOT}/third_party/SDL2/include")
if(NOT EXISTS "${SDL2_INCLUDE_ROOT}/SDL2/SDL.h")
    message(FATAL_ERROR "SDL2 headers missing. Run android_tv/prepare_sdl2.sh before building.")
endif()

set(SDL2_LIB "${CMAKE_CURRENT_LIST_DIR}/../jniLibs/${ANDROID_ABI}/libSDL2.so")
if(NOT EXISTS "${SDL2_LIB}")
    message(FATAL_ERROR "libSDL2.so for ${ANDROID_ABI} missing. Run android_tv/prepare_sdl2.sh before building.")
endif()

add_library(SDL2 SHARED IMPORTED)
set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION "${SDL2_LIB}")

add_library(main SHARED "${REPO_ROOT}/src/main_android_tv.c")

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
)

target_link_libraries(main
    SDL2
    GLESv2
    EGL
    android
    log
    z
    m
)
