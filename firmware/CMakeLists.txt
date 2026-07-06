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
    set(sdkVersion 2.3.0)
    set(toolchainVersion 15_2_Rel1)
    set(picotoolVersion 2.3.0-a4)
    set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
    if (EXISTS ${picoVscode})
        include(${picoVscode})
    endif()

    if (PICO_TOOLCHAIN_PATH)
        # Recover from empty compiler paths injected by some kits.
        if (DEFINED CMAKE_C_COMPILER AND CMAKE_C_COMPILER STREQUAL "")
            set(_pico_cc "${PICO_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc")
            if (EXISTS "${_pico_cc}")
                set(CMAKE_C_COMPILER "${_pico_cc}" CACHE FILEPATH "C compiler" FORCE)
            endif()
            unset(_pico_cc)
        endif()
        if (DEFINED CMAKE_CXX_COMPILER AND CMAKE_CXX_COMPILER STREQUAL "")
            set(_pico_cxx "${PICO_TOOLCHAIN_PATH}/bin/arm-none-eabi-g++")
            if (EXISTS "${_pico_cxx}")
                set(CMAKE_CXX_COMPILER "${_pico_cxx}" CACHE FILEPATH "C++ compiler" FORCE)
            endif()
            unset(_pico_cxx)
        endif()
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

    # Protocol setup from ../protocol. Firmware must link COBS built for
    # arm-none-eabi; a host /usr/local/lib/libcobs.a cannot be linked by Pico.
    set(_firmware_cobs_root "$ENV{HOME}/.local/pico-cobs")
    if (EXISTS "${_firmware_cobs_root}/lib/libcobs.a")
        set(COBS_ROOT "${_firmware_cobs_root}" CACHE PATH "" FORCE)
        set(COBS_INCLUDE_DIR "${_firmware_cobs_root}/include" CACHE PATH "" FORCE)
        set(COBS_LIBRARY "${_firmware_cobs_root}/lib/libcobs.a" CACHE FILEPATH "" FORCE)
    else()
        set(COBS_ROOT "/usr/local" CACHE PATH "" FORCE)
        set(COBS_INCLUDE_DIR "/usr/local/include" CACHE PATH "" FORCE)
        set(COBS_LIBRARY "/usr/local/lib/libcobs.a" CACHE FILEPATH "" FORCE)
    endif()
    unset(_firmware_cobs_root)

    set(_firmware_crcpp_root "$ENV{HOME}/.local")
    if (EXISTS "${_firmware_crcpp_root}/include/CRC.h")
        set(CRCPP_ROOT "${_firmware_crcpp_root}" CACHE PATH "" FORCE)
        set(CRCPP_INCLUDE_DIR "${_firmware_crcpp_root}/include" CACHE PATH "" FORCE)
    else()
        set(CRCPP_ROOT "/usr/local" CACHE PATH "" FORCE)
        set(CRCPP_INCLUDE_DIR "/usr/local/include" CACHE PATH "" FORCE)
    endif()
    unset(_firmware_crcpp_root)
    set(EASYLORA_ENABLE_PROTOBUF OFF CACHE BOOL "" FORCE)
    set(EASYLORA_ENABLE_NANOPB OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../protocol ${CMAKE_BINARY_DIR}/protocol)

    include(${CMAKE_CURRENT_LIST_DIR}/../messages/EasyLoRaMessages.cmake)
    set(NANOPB_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/nanopb")
    easylora_generate_nanopb(NANOPB_GEN_SOURCES NANOPB_GEN_HEADERS "${NANOPB_GEN_DIR}")
    add_custom_target(firmware_generate_nanopb DEPENDS ${NANOPB_GEN_SOURCES} ${NANOPB_GEN_HEADERS})

    # Add executable. Default name is the project name, version 0.1
    add_executable(firmware
        main.cpp
        lib/UARTWrapper/UARTWrapper.cpp
        lib/FrameWaiter/FrameWaiter.hpp
        lib/ByteTransport/ByteTransport.hpp
        include/ValidPins.hpp
        lib/Core_1/Core_1.cpp
        lib/Core_0/Core_0.hpp
        lib/Core_0/Core_0.cpp
        lib/FrameResponder/FrameResponder.cpp
        lib/GetDeviceInfoHandler/GetDeviceInfoHandler.cpp
        ${NANOPB_GEN_SOURCES}
        ${NANOPB_GEN_HEADERS}
        "${EASYLORA_NANOPB_SOURCE_DIR}/pb_common.c"
        "${EASYLORA_NANOPB_SOURCE_DIR}/pb_decode.c"
        "${EASYLORA_NANOPB_SOURCE_DIR}/pb_encode.c"
    )
    add_dependencies(firmware firmware_generate_nanopb)

    pico_set_program_name(firmware "firmware")
    pico_set_program_version(firmware "0.1")

    # Modify the below lines to enable/disable output over UART/USB
    pico_enable_stdio_uart(firmware 1)
    pico_enable_stdio_usb(firmware 1)

    # Add the standard library to the build
    target_link_libraries(firmware PRIVATE
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
        ${CMAKE_CURRENT_LIST_DIR}/lib/Core_1
        ${CMAKE_CURRENT_LIST_DIR}/lib/Core_0
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/lib/GetDeviceInfoHandler
        ${CMAKE_CURRENT_LIST_DIR}/lib/FrameResponder
        ${NANOPB_GEN_DIR}
        ${EASYLORA_NANOPB_SOURCE_DIR}
    )

    # Add any user requested libraries
    target_link_libraries(firmware PRIVATE
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

    include(${CMAKE_CURRENT_LIST_DIR}/../messages/EasyLoRaMessages.cmake)
    set(NANOPB_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/nanopb")
    easylora_generate_nanopb(NANOPB_GEN_SOURCES NANOPB_GEN_HEADERS "${NANOPB_GEN_DIR}")
    add_custom_target(firmware_generate_nanopb DEPENDS ${NANOPB_GEN_SOURCES} ${NANOPB_GEN_HEADERS})

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

    add_executable(getdeviceinfohandler_tests
        tests/getdeviceinfohandler_tests.cpp
        lib/GetDeviceInfoHandler/GetDeviceInfoHandler.cpp
        ${NANOPB_GEN_SOURCES}
        ${NANOPB_GEN_HEADERS}
        "${EASYLORA_NANOPB_SOURCE_DIR}/pb_common.c"
        "${EASYLORA_NANOPB_SOURCE_DIR}/pb_decode.c"
        "${EASYLORA_NANOPB_SOURCE_DIR}/pb_encode.c"
    )
    add_dependencies(getdeviceinfohandler_tests firmware_generate_nanopb)
    target_compile_features(getdeviceinfohandler_tests PRIVATE cxx_std_23)
    target_include_directories(getdeviceinfohandler_tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/lib/GetDeviceInfoHandler
        ${CMAKE_CURRENT_LIST_DIR}/../protocol/include
        ${NANOPB_GEN_DIR}
        ${EASYLORA_NANOPB_SOURCE_DIR}
    )
    target_link_libraries(getdeviceinfohandler_tests PRIVATE GTest::gtest_main)

    if (FIRMWARE_TEST_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(framewaiter_tests PRIVATE
            -fsanitize=${FIRMWARE_TEST_SANITIZERS}
            -fno-omit-frame-pointer
            -g
        )
        target_link_options(framewaiter_tests PRIVATE -fsanitize=${FIRMWARE_TEST_SANITIZERS})

        target_compile_options(getdeviceinfohandler_tests PRIVATE
            -fsanitize=${FIRMWARE_TEST_SANITIZERS}
            -fno-omit-frame-pointer
            -g
        )
        target_link_options(getdeviceinfohandler_tests PRIVATE -fsanitize=${FIRMWARE_TEST_SANITIZERS})
    endif()

    add_test(NAME framewaiter_tests COMMAND framewaiter_tests)
    add_test(NAME getdeviceinfohandler_tests COMMAND getdeviceinfohandler_tests)
endif()
