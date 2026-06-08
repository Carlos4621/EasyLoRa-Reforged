cmake_minimum_required(VERSION 3.20)

project(EasyLoRaProtocol LANGUAGES C CXX)

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

# Protobuf / nanopb code generation
option(EASYLORA_ENABLE_PROTOBUF "Generate protobuf code" ON)
option(EASYLORA_ENABLE_NANOPB "Generate nanopb code" ON)

set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated")
set(PROTOBUF_GEN_DIR "${GENERATED_DIR}/protobuff")
set(NANOPB_GEN_DIR "${GENERATED_DIR}/nanopb")

file(GLOB PROTO_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")

set(GENERATED_OUTPUTS "")

if((EASYLORA_ENABLE_PROTOBUF OR EASYLORA_ENABLE_NANOPB) AND PROTO_FILES)
	file(MAKE_DIRECTORY "${PROTOBUF_GEN_DIR}" "${NANOPB_GEN_DIR}")
elseif(EASYLORA_ENABLE_PROTOBUF OR EASYLORA_ENABLE_NANOPB)
	message(STATUS "No .proto files found in proto/; skipping code generation")
endif()

if(EASYLORA_ENABLE_PROTOBUF)
	set(Protobuf_ROOT "" CACHE PATH "Root path for Protobuf install")
	find_package(Protobuf REQUIRED)
	if(NOT Protobuf_PROTOC_EXECUTABLE)
		message(FATAL_ERROR "protoc not found. Install protobuf-compiler or set Protobuf_ROOT.")
	endif()

	foreach(PROTO_FILE IN LISTS PROTO_FILES)
		get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)
		list(APPEND PROTOBUF_OUTPUTS
			"${PROTOBUF_GEN_DIR}/${PROTO_NAME}.pb.cc"
			"${PROTOBUF_GEN_DIR}/${PROTO_NAME}.pb.h"
		)
		add_custom_command(
			OUTPUT
				"${PROTOBUF_GEN_DIR}/${PROTO_NAME}.pb.cc"
				"${PROTOBUF_GEN_DIR}/${PROTO_NAME}.pb.h"
			COMMAND "${Protobuf_PROTOC_EXECUTABLE}"
				--proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
				--cpp_out=${PROTOBUF_GEN_DIR}
				${PROTO_FILE}
			DEPENDS "${PROTO_FILE}"
			COMMENT "Generating Protobuf for ${PROTO_NAME}"
			VERBATIM
		)
	endforeach()

	list(APPEND GENERATED_OUTPUTS ${PROTOBUF_OUTPUTS})
endif()

if(EASYLORA_ENABLE_NANOPB)
	set(NANOPB_ROOT "" CACHE PATH "Root path for nanopb source (optional)")
	if(NANOPB_ROOT)
		set(NANOPB_SOURCE_DIR "${NANOPB_ROOT}")
	else()
		include(FetchContent)
		FetchContent_Declare(
			nanopb
			GIT_REPOSITORY https://github.com/nanopb/nanopb.git
			GIT_TAG 0.4.8
		)
		FetchContent_MakeAvailable(nanopb)
		set(NANOPB_SOURCE_DIR "${nanopb_SOURCE_DIR}")
	endif()

	set(NANOPB_PLUGIN "${NANOPB_SOURCE_DIR}/generator/protoc-gen-nanopb")
	if(NOT EXISTS "${NANOPB_PLUGIN}")
		message(FATAL_ERROR "nanopb plugin not found at ${NANOPB_PLUGIN}")
	endif()

	if(NOT Protobuf_PROTOC_EXECUTABLE)
		find_package(Protobuf REQUIRED)
	endif()

	foreach(PROTO_FILE IN LISTS PROTO_FILES)
		get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)
		set(PROTO_OPTIONS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proto/${PROTO_NAME}.options")
		set(NANOPB_DEPENDS "${PROTO_FILE}")
		if(EXISTS "${PROTO_OPTIONS_FILE}")
			list(APPEND NANOPB_DEPENDS "${PROTO_OPTIONS_FILE}")
		endif()
		list(APPEND NANOPB_OUTPUTS
			"${NANOPB_GEN_DIR}/${PROTO_NAME}.pb.c"
			"${NANOPB_GEN_DIR}/${PROTO_NAME}.pb.h"
		)
		add_custom_command(
			OUTPUT
				"${NANOPB_GEN_DIR}/${PROTO_NAME}.pb.c"
				"${NANOPB_GEN_DIR}/${PROTO_NAME}.pb.h"
			COMMAND "${Protobuf_PROTOC_EXECUTABLE}"
				--proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
				--plugin=protoc-gen-nanopb=${NANOPB_PLUGIN}
				--nanopb_opt=-I${CMAKE_CURRENT_SOURCE_DIR}/proto
				--nanopb_out=${NANOPB_GEN_DIR}
				${PROTO_FILE}
			DEPENDS ${NANOPB_DEPENDS}
			COMMENT "Generating nanopb for ${PROTO_NAME}"
			VERBATIM
		)
	endforeach()

	list(APPEND GENERATED_OUTPUTS ${NANOPB_OUTPUTS})
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
)
add_library(EasyLoRa::protocol ALIAS easylora_protocol)

if(TARGET generate_protocol_code)
	add_dependencies(easylora_protocol generate_protocol_code)
endif()

target_include_directories(easylora_protocol PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${PROTOBUF_GEN_DIR}>
	$<BUILD_INTERFACE:${NANOPB_GEN_DIR}>
	$<INSTALL_INTERFACE:include>
)

target_link_libraries(easylora_protocol PUBLIC cobs::cobs)
target_link_libraries(easylora_protocol PUBLIC crcpp::crcpp)
if(TARGET nanopb)
	target_link_libraries(easylora_protocol PUBLIC nanopb)
endif()

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
