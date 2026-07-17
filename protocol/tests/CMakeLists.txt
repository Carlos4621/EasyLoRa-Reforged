find_package(GTest CONFIG REQUIRED)

add_executable(${PROJECT_NAME}_tests
    test_crc_calculator.cpp
)

target_link_libraries(${PROJECT_NAME}_tests
    PRIVATE
        ${PROJECT_NAME}::${PROJECT_NAME}
        GTest::gtest_main
)

target_compile_features(${PROJECT_NAME}_tests PRIVATE cxx_std_23)
set_target_properties(${PROJECT_NAME}_tests PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(${PROJECT_NAME}_tests
    PRIVATE
        "${PROJECT_SOURCE_DIR}/src/detail"
        ${CRCPP_INCLUDE_DIRS}
)

project_apply_options(${PROJECT_NAME}_tests)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)
