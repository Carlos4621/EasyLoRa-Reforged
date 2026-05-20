# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(firmware C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Protocol setup from ../protocol
set(COBS_ROOT "/usr/local" CACHE PATH "" FORCE)
set(COBS_INCLUDE_DIR "/usr/local/include/cobs" CACHE PATH "" FORCE)
set(COBS_LIBRARY "/usr/local/lib/libcobs.a" CACHE FILEPATH "" FORCE)
set(CRCPP_ROOT "/usr/local" CACHE PATH "" FORCE)
set(CRCPP_INCLUDE_DIR "/usr/local/include" CACHE PATH "" FORCE)
set(EASYLORA_ENABLE_PROTOBUF OFF CACHE BOOL "" FORCE)
set(EASYLORA_ENABLE_NANOPB OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../protocol ${CMAKE_BINARY_DIR}/protocol)

# Add executable. Default name is the project name, version 0.1

add_executable(firmware
    main.cpp
    lib/UARTWrapper/UARTWrapper.cpp
    include/Instances.hpp
    include/UART_IRQ.hpp
    include/ValidPins.hpp
)

pico_set_program_name(firmware "firmware")
pico_set_program_version(firmware "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(firmware 1)
pico_enable_stdio_usb(firmware 1)

# Add the standard library to the build
target_link_libraries(firmware
    pico_stdlib
    EasyLoRa::protocol)

# Add the standard include files to the build
target_include_directories(firmware PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/lib/ByteTransport
    ${CMAKE_CURRENT_LIST_DIR}/lib/Ring-Buffer
    ${CMAKE_CURRENT_LIST_DIR}/lib/UARTWrapper
    ${CMAKE_CURRENT_LIST_DIR}/lib/picoPins/include
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Add any user requested libraries
target_link_libraries(firmware 
        hardware_i2c
        )

pico_add_extra_outputs(firmware)

