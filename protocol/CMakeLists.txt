cmake_minimum_required(VERSION 3.20)

project(EasyLoRaProtocol LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(POLICY CMP0135)
	cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED EASYLORA_BUILD_EXAMPLES)
	option(EASYLORA_BUILD_EXAMPLES "Build examples" OFF)
endif()

if(NOT DEFINED EASYLORA_BUILD_TESTS)
	option(EASYLORA_BUILD_TESTS "Build tests" OFF)
endif()

option(EASYLORA_ENABLE_WARNINGS "Enable strict compiler warnings for EasyLoRa protocol targets" OFF)
option(EASYLORA_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer for EasyLoRa protocol targets" OFF)
option(EASYLORA_ENABLE_PROTOBUF "Generate and expose C++ Protobuf message classes" ON)
option(EASYLORA_ENABLE_NANOPB "Generate and expose nanopb message classes" ON)

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

# Protobuf / nanopb code generation. Custom command outputs are reused when
# present and regenerated when a schema or nanopb options file changes.
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(PROTOBUF_GEN_DIR "${GENERATED_DIR}/protobuf")
set(NANOPB_GEN_DIR "${GENERATED_DIR}/nanopb")
include("${CMAKE_CURRENT_SOURCE_DIR}/../messages/EasyLoRaMessages.cmake")

set(PROTOBUF_OUTPUTS "")
set(PROTOBUF_HEADERS "")
set(NANOPB_OUTPUTS "")
set(NANOPB_HEADERS "")
set(GENERATED_OUTPUTS "")

set(Protobuf_ROOT "" CACHE PATH "Root path for Protobuf install")
set(NANOPB_ROOT "" CACHE PATH "Root path for nanopb source (optional)")

if(EASYLORA_ENABLE_PROTOBUF)
	easylora_generate_protobuf(PROTOBUF_OUTPUTS PROTOBUF_HEADERS "${PROTOBUF_GEN_DIR}")
	list(APPEND GENERATED_OUTPUTS ${PROTOBUF_OUTPUTS} ${PROTOBUF_HEADERS})
endif()

if(EASYLORA_ENABLE_NANOPB)
	easylora_generate_nanopb(NANOPB_OUTPUTS NANOPB_HEADERS "${NANOPB_GEN_DIR}")
	list(APPEND GENERATED_OUTPUTS ${NANOPB_OUTPUTS} ${NANOPB_HEADERS})
endif()

if(GENERATED_OUTPUTS)
	add_custom_target(generate_protocol_code ALL DEPENDS ${GENERATED_OUTPUTS})
endif()

add_library(easylora_protocol
	src/CobsrCodec.cpp
	src/CobsrDecoder.cpp
	src/FrameCodec.cpp
	src/FrameDecoder.cpp
	src/ProtocolCodec.cpp
	src/ProtocolDecoder.cpp
	${PROTOBUF_OUTPUTS}
	${NANOPB_OUTPUTS}
)

if(EASYLORA_ENABLE_NANOPB)
	target_sources(easylora_protocol PRIVATE
		"${EASYLORA_NANOPB_SOURCE_DIR}/pb_common.c"
		"${EASYLORA_NANOPB_SOURCE_DIR}/pb_decode.c"
		"${EASYLORA_NANOPB_SOURCE_DIR}/pb_encode.c"
	)
endif()
add_library(EasyLoRa::protocol ALIAS easylora_protocol)
set_target_properties(easylora_protocol PROPERTIES EXPORT_NAME protocol)

if(TARGET generate_protocol_code)
	add_dependencies(easylora_protocol generate_protocol_code)
endif()

target_include_directories(easylora_protocol PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

if(EASYLORA_ENABLE_PROTOBUF)
	target_include_directories(easylora_protocol PUBLIC
		$<BUILD_INTERFACE:${PROTOBUF_GEN_DIR}>
		$<INSTALL_INTERFACE:include/EasyLoRa/protobuf>
	)
endif()

if(EASYLORA_ENABLE_NANOPB)
	target_include_directories(easylora_protocol PUBLIC
		$<BUILD_INTERFACE:${NANOPB_GEN_DIR}>
		$<BUILD_INTERFACE:${EASYLORA_NANOPB_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include/EasyLoRa/nanopb>
	)
endif()

target_link_libraries(easylora_protocol PUBLIC cobs::cobs)
target_link_libraries(easylora_protocol PUBLIC crcpp::crcpp)
if(EASYLORA_ENABLE_PROTOBUF)
	target_link_libraries(easylora_protocol PUBLIC protobuf::libprotobuf)
endif()

if(EASYLORA_ENABLE_WARNINGS)
	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(easylora_protocol PRIVATE -Wall -Wextra -Wpedantic)
	elseif(MSVC)
		target_compile_options(easylora_protocol PRIVATE /W4)
	endif()
endif()

if(EASYLORA_ENABLE_SANITIZERS)
	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(easylora_protocol PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
		target_link_options(easylora_protocol PUBLIC -fsanitize=address,undefined)
	else()
		message(WARNING "EASYLORA_ENABLE_SANITIZERS is only configured for GCC and Clang")
	endif()
endif()

install(TARGETS easylora_protocol
	EXPORT EasyLoRaProtocolTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT EasyLoRaProtocol
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT EasyLoRaProtocol
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT EasyLoRaProtocol
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	COMPONENT EasyLoRaProtocol
	FILES_MATCHING PATTERN "*.hpp"
)

if(EASYLORA_ENABLE_PROTOBUF)
	install(DIRECTORY "${PROTOBUF_GEN_DIR}/"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/EasyLoRa/protobuf"
		COMPONENT EasyLoRaProtocol
		FILES_MATCHING PATTERN "*.h"
	)
endif()

if(EASYLORA_ENABLE_NANOPB)
	install(DIRECTORY "${NANOPB_GEN_DIR}/"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/EasyLoRa/nanopb"
		COMPONENT EasyLoRaProtocol
		FILES_MATCHING PATTERN "*.h"
	)

	install(FILES
		"${EASYLORA_NANOPB_SOURCE_DIR}/pb.h"
		"${EASYLORA_NANOPB_SOURCE_DIR}/pb_common.h"
		"${EASYLORA_NANOPB_SOURCE_DIR}/pb_decode.h"
		"${EASYLORA_NANOPB_SOURCE_DIR}/pb_encode.h"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/EasyLoRa/nanopb"
		COMPONENT EasyLoRaProtocol
	)
endif()

install(EXPORT EasyLoRaProtocolTargets
	FILE EasyLoRaProtocolTargets.cmake
	NAMESPACE EasyLoRa::
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/EasyLoRaProtocol"
	COMPONENT EasyLoRaProtocol
)

configure_package_config_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/EasyLoRaProtocolConfig.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/EasyLoRaProtocolConfig.cmake"
	INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/EasyLoRaProtocol"
)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/EasyLoRaProtocolConfigVersion.cmake"
	VERSION 0.1.0
	COMPATIBILITY SameMajorVersion
)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/EasyLoRaProtocolConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/EasyLoRaProtocolConfigVersion.cmake"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/EasyLoRaProtocol"
	COMPONENT EasyLoRaProtocol
)

if(EASYLORA_BUILD_EXAMPLES)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
		add_subdirectory(examples)
	else()
		message(STATUS "examples/CMakeLists.txt not found; examples not built")
	endif()
endif()

if(EASYLORA_BUILD_TESTS)
	include(CTest)
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
