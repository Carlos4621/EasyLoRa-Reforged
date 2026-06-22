cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(EasyLoRaApi LANGUAGES CXX)

find_package(Boost CONFIG REQUIRED)

option(EASYLORA_API_BUILD_TESTS "Build EasyLoRa API tests" OFF)

set(EASYLORA_ENABLE_PROTOBUF ON CACHE BOOL "" FORCE)
set(EASYLORA_ENABLE_NANOPB OFF CACHE BOOL "" FORCE)
add_subdirectory(
	${CMAKE_CURRENT_SOURCE_DIR}/../protocol
	${CMAKE_CURRENT_BINARY_DIR}/protocol
)

add_library(easylora_api
	src/EasyLoRa.cpp
	include/SerialTransport.hpp
)

add_library(EasyLoRa::api ALIAS easylora_api)

target_include_directories(easylora_api PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/protocol/generated/protobuf>
)

target_link_libraries(easylora_api PUBLIC
	Boost::headers
	EasyLoRa::protocol
)

if(EASYLORA_API_BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()
