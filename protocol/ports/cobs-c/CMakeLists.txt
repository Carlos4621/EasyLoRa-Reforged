cmake_minimum_required(VERSION 3.20)

project(cobs-c
    VERSION 0.5.0
    LANGUAGES C
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(cobs-c
    cobs.c
    cobsr.c
)

add_library(cobs-c::cobs-c ALIAS cobs-c)

target_include_directories(cobs-c
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

set_target_properties(cobs-c PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO

    # Necesario al construir una DLL con MSVC porque la biblioteca
    # original no utiliza macros __declspec(dllexport).
    WINDOWS_EXPORT_ALL_SYMBOLS YES

    EXPORT_NAME cobs-c
)

install(
    TARGETS cobs-c
    EXPORT cobs-c-targets

    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"

    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
    FILES
        cobs.h
        cobsr.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
    EXPORT cobs-c-targets
    FILE cobs-c-targets.cmake
    NAMESPACE cobs-c::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cobs-c"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cobs-c-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cobs-c-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cobs-c"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cobs-c-config-version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/cobs-c-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/cobs-c-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cobs-c"
)