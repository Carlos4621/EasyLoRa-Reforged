cmake_minimum_required(VERSION 3.20)

project(EasyLoRaProtocolExternalConsumer LANGUAGES C CXX)

if(NOT DEFINED PROTOCOL_SOURCE_DIR)
	message(FATAL_ERROR "PROTOCOL_SOURCE_DIR is required")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EASYLORA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(EASYLORA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory("${PROTOCOL_SOURCE_DIR}" easylora_protocol)

add_executable(external_consumer main.cpp)
target_link_libraries(external_consumer PRIVATE EasyLoRa::protocol)
