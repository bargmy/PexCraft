cmake_minimum_required(VERSION 3.8)
project(pexcraft_lgwebos VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)

add_executable(pexcraft_lgwebos ../main_lgwebos.c)

target_compile_definitions(pexcraft_lgwebos PRIVATE
    PEX_PLATFORM_LGWEBOS=1
    PEX_PLATFORM_SDL2=1
)

target_include_directories(pexcraft_lgwebos SYSTEM PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
)

target_link_libraries(pexcraft_lgwebos PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    GLESv2
    EGL
    z
    m
    pthread
)

configure_file(appinfo.json.in ${CMAKE_BINARY_DIR}/appinfo.json @ONLY)

install(TARGETS pexcraft_lgwebos RUNTIME DESTINATION .)
install(FILES ${CMAKE_BINARY_DIR}/appinfo.json DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/generated/icon.png DESTINATION . RENAME icon.png OPTIONAL)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/../assets DESTINATION . OPTIONAL)

set(CPACK_GENERATOR "External")
set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${CMAKE_SOURCE_DIR}/AresPackage.cmake")
set(CPACK_EXTERNAL_ENABLE_STAGING TRUE)
set(CPACK_MONOLITHIC_INSTALL TRUE)
include(CPack)
