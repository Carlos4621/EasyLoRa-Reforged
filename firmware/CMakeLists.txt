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

    # Protocol setup from ../protocol
    set(COBS_ROOT "/usr/local" CACHE PATH "" FORCE)
    set(COBS_INCLUDE_DIR "/usr/local/include/cobs" CACHE PATH "" FORCE)
    set(COBS_LIBRARY "/usr/local/lib/libcobs.a" CACHE FILEPATH "" FORCE)
    set(CRCPP_ROOT "/usr/local" CACHE PATH "" FORCE)
    set(CRCPP_INCLUDE_DIR "/usr/local/include" CACHE PATH "" FORCE)
    set(EASYLORA_ENABLE_PROTOBUF OFF CACHE BOOL "" FORCE)
    set(EASYLORA_ENABLE_NANOPB OFF CACHE BOOL "" FORCE)
    if (NOT TARGET protobuf-nanopb-static AND NOT TARGET protobuf-nanopb)
        include(FetchContent)
        FetchContent_Declare(
            nanopb
            GIT_REPOSITORY https://github.com/nanopb/nanopb.git
            GIT_TAG 0.4.8
        )
        FetchContent_MakeAvailable(nanopb)
    endif()
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../protocol ${CMAKE_BINARY_DIR}/protocol)

    set(NANOPB_GEN_DIR "${CMAKE_CURRENT_LIST_DIR}/../protocol/generated/nanopb")
    file(GLOB NANOPB_GEN_SOURCES CONFIGURE_DEPENDS "${NANOPB_GEN_DIR}/*.pb.c")

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
    )

    target_sources(firmware PRIVATE ${NANOPB_GEN_SOURCES})

    pico_set_program_name(firmware "firmware")
    pico_set_program_version(firmware "0.1")

    # Modify the below lines to enable/disable output over UART/USB
    pico_enable_stdio_uart(firmware 1)
    pico_enable_stdio_usb(firmware 1)

    # Add the standard library to the build
    target_link_libraries(firmware PRIVATE
        pico_stdlib
        EasyLoRa::protocol)
    if (TARGET protobuf-nanopb-static)
        target_link_libraries(firmware PRIVATE protobuf-nanopb-static)
    elseif (TARGET protobuf-nanopb)
        target_link_libraries(firmware PRIVATE protobuf-nanopb)
    endif()

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

    if (NOT TARGET protobuf-nanopb-static AND NOT TARGET protobuf-nanopb)
        FetchContent_Declare(
            nanopb
            GIT_REPOSITORY https://github.com/nanopb/nanopb.git
            GIT_TAG 0.4.8
        )
        FetchContent_MakeAvailable(nanopb)
    endif()

    find_package(Protobuf REQUIRED)

    set(NANOPB_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/nanopb")
    set(DEVICE_INFO_PROTO "${CMAKE_CURRENT_LIST_DIR}/../protocol/proto/DeviceInfo.proto")
    set(DEVICE_INFO_OPTIONS "${CMAKE_CURRENT_LIST_DIR}/../protocol/proto/DeviceInfo.options")
    set(DEVICE_INFO_NANOPB_C "${NANOPB_GEN_DIR}/DeviceInfo.pb.c")
    set(DEVICE_INFO_NANOPB_H "${NANOPB_GEN_DIR}/DeviceInfo.pb.h")
    set(NANOPB_GEN_SOURCES "${DEVICE_INFO_NANOPB_C}")
    set(NANOPB_PLUGIN "${nanopb_SOURCE_DIR}/generator/protoc-gen-nanopb")

    file(MAKE_DIRECTORY "${NANOPB_GEN_DIR}")
    add_custom_command(
        OUTPUT
            "${DEVICE_INFO_NANOPB_C}"
            "${DEVICE_INFO_NANOPB_H}"
        COMMAND "${Protobuf_PROTOC_EXECUTABLE}"
            --proto_path=${CMAKE_CURRENT_LIST_DIR}/../protocol/proto
            --plugin=protoc-gen-nanopb=${NANOPB_PLUGIN}
            --nanopb_opt=-I${CMAKE_CURRENT_LIST_DIR}/../protocol/proto
            --nanopb_out=${NANOPB_GEN_DIR}
            ${DEVICE_INFO_PROTO}
        DEPENDS
            "${DEVICE_INFO_PROTO}"
            "${DEVICE_INFO_OPTIONS}"
        COMMENT "Generating nanopb for DeviceInfo"
        VERBATIM
    )
    add_custom_target(firmware_generate_nanopb DEPENDS "${DEVICE_INFO_NANOPB_C}" "${DEVICE_INFO_NANOPB_H}")

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
    )
    add_dependencies(getdeviceinfohandler_tests firmware_generate_nanopb)
    target_compile_features(getdeviceinfohandler_tests PRIVATE cxx_std_23)
    target_include_directories(getdeviceinfohandler_tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/lib/GetDeviceInfoHandler
        ${CMAKE_CURRENT_LIST_DIR}/../protocol/include
        ${NANOPB_GEN_DIR}
    )
    target_link_libraries(getdeviceinfohandler_tests PRIVATE GTest::gtest_main)
    if (TARGET protobuf-nanopb-static)
        target_link_libraries(getdeviceinfohandler_tests PRIVATE protobuf-nanopb-static)
    elseif (TARGET protobuf-nanopb)
        target_link_libraries(getdeviceinfohandler_tests PRIVATE protobuf-nanopb)
    endif()

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
