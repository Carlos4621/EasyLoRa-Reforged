# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FIRMWARE_BUILD_TESTS "Build host-side unit tests" OFF)
set(FIRMWARE_TEST_SANITIZERS "address,undefined,leak" CACHE STRING "Sanitizers to enable for tests (empty to disable or set to 'thread')")

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

if (NOT FIRMWARE_BUILD_TESTS)
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
endif()

project(firmware C CXX ASM)

if (NOT FIRMWARE_BUILD_TESTS)
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
        lib/FrameWaiter/FrameWaiter.hpp
        lib/ByteTransport/ByteTransport.hpp
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
        ${CMAKE_CURRENT_LIST_DIR}/lib/FrameWaiter
        ${CMAKE_CURRENT_LIST_DIR}/lib/picoPins/include
        ${CMAKE_CURRENT_LIST_DIR}/include
    )

    # Add any user requested libraries
    target_link_libraries(firmware 
            hardware_i2c
            )

    pico_add_extra_outputs(firmware)
endif()

if (FIRMWARE_BUILD_TESTS)
    include(FetchContent)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(framewaiter_tests
        tests/framewaiter_tests.cpp
    )
    target_compile_features(framewaiter_tests PRIVATE cxx_std_23)
    target_include_directories(framewaiter_tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/lib/FrameWaiter
        ${CMAKE_CURRENT_LIST_DIR}/lib/Ring-Buffer
    )
    target_link_libraries(framewaiter_tests PRIVATE GTest::gtest_main)

    if (FIRMWARE_TEST_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(framewaiter_tests PRIVATE
            -fsanitize=${FIRMWARE_TEST_SANITIZERS}
            -fno-omit-frame-pointer
            -g
        )
        target_link_options(framewaiter_tests PRIVATE -fsanitize=${FIRMWARE_TEST_SANITIZERS})
    endif()

    add_test(NAME framewaiter_tests COMMAND framewaiter_tests)
endif()

