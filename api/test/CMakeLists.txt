include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(serial_transport_tests
    SerialTransportTests.cpp
)

target_compile_features(serial_transport_tests PRIVATE cxx_std_23)
target_link_libraries(serial_transport_tests PRIVATE GTest::gtest_main EasyLoRa::api)

include(GoogleTest)
gtest_discover_tests(serial_transport_tests)
