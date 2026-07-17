cmake_minimum_required(VERSION 3.28)

project(
    protocol
    VERSION 0.1.0
    DESCRIPTION "Protocolo de comunicación serial"
    LANGUAGES CXX
)

# Evita requerir clang-scan-deps cuando el proyecto no utiliza módulos C++20.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

option(PROTOCOL_BUILD_TESTS "Compilar pruebas unitarias" ON)
option(PROTOCOL_WARNINGS_AS_ERRORS "Tratar advertencias como errores" OFF)
option(PROTOCOL_ENABLE_CLANG_TIDY "Activar clang-tidy" OFF)
option(PROTOCOL_ENABLE_CPPCHECK "Activar cppcheck" OFF)
option(PROTOCOL_ENABLE_ASAN "Activar AddressSanitizer" OFF)
option(PROTOCOL_ENABLE_UBSAN "Activar UndefinedBehaviorSanitizer" OFF)
option(PROTOCOL_ENABLE_TSAN "Activar ThreadSanitizer" OFF)
option(PROTOCOL_ENABLE_COVERAGE "Activar cobertura" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ProjectOptions)

add_library(${PROJECT_NAME}
    src/Header.cpp
    src/ProtocolEncoder.cpp
    src/detail/CrcCalculator.cpp
)

find_path(CRCPP_INCLUDE_DIRS "CRC.h" REQUIRED)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME}
    PUBLIC
        cxx_std_23
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CRCPP_INCLUDE_DIRS}
)

generate_export_header(${PROJECT_NAME}
    BASE_NAME protocol
    EXPORT_FILE_NAME
        "${CMAKE_CURRENT_BINARY_DIR}/generated/protocol/protocol_export.hpp"
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        DEBUG_POSTFIX "d"
)

project_apply_options(${PROJECT_NAME})

if(PROTOCOL_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ProjectConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/generated/protocol/protocol_export.hpp"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/protocol"
)

install(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)
