cmake_minimum_required(VERSION 3.20)

project(EasyLoRaProtocol LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(export_compile_commands ON)

if(NOT DEFINED EASYLORA_BUILD_EXAMPLES)
	option(EASYLORA_BUILD_EXAMPLES "Build examples" OFF)
endif()

if(NOT DEFINED EASYLORA_BUILD_TESTS)
	option(EASYLORA_BUILD_TESTS "Build tests" OFF)
endif()

# COBS (installed via make install)
set(COBS_ROOT "" CACHE PATH "Root path for COBS install")
find_path(COBS_INCLUDE_DIR
	NAMES cobs.h
	HINTS ${COBS_ROOT}
	PATH_SUFFIXES include include/cobs
)
find_library(COBS_LIBRARY
	NAMES cobs
	HINTS ${COBS_ROOT}
	PATH_SUFFIXES lib lib64
)
if(NOT COBS_INCLUDE_DIR OR NOT COBS_LIBRARY)
	message(FATAL_ERROR "COBS not found. Set COBS_ROOT or ensure cobs is installed.")
endif()

add_library(cobs::cobs UNKNOWN IMPORTED)
set_target_properties(cobs::cobs PROPERTIES
	IMPORTED_LOCATION "${COBS_LIBRARY}"
	INTERFACE_INCLUDE_DIRECTORIES "${COBS_INCLUDE_DIR}"
)

# CRCpp (header-only)
set(CRCPP_ROOT "" CACHE PATH "Root path for CRCpp install")
find_path(CRCPP_INCLUDE_DIR
	NAMES CRC.h CRCpp.h
	HINTS ${CRCPP_ROOT}
	PATH_SUFFIXES include
)
if(NOT CRCPP_INCLUDE_DIR)
	message(FATAL_ERROR "CRCpp not found. Set CRCPP_ROOT or ensure CRCpp is installed.")
endif()

add_library(crcpp::crcpp INTERFACE IMPORTED)
set_target_properties(crcpp::crcpp PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${CRCPP_INCLUDE_DIR}"
)

# Header-only base target until sources are added.
add_library(easylora_protocol INTERFACE)
add_library(EasyLoRa::protocol ALIAS easylora_protocol)

target_include_directories(easylora_protocol INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

target_link_libraries(easylora_protocol INTERFACE cobs::cobs)
target_link_libraries(easylora_protocol INTERFACE crcpp::crcpp)

if(EASYLORA_BUILD_EXAMPLES)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
		add_subdirectory(examples)
	else()
		message(STATUS "examples/CMakeLists.txt not found; examples not built")
	endif()
endif()

if(EASYLORA_BUILD_TESTS)
	include(FetchContent)
	set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
	set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(
		googletest
		URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
	)
	FetchContent_MakeAvailable(googletest)

	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
		enable_testing()
		add_subdirectory(test)
	else()
		message(STATUS "test/CMakeLists.txt not found; tests not built")
	endif()
endif()
