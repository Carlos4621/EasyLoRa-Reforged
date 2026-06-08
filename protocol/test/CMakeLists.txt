add_executable(raw_frame_tests
	RawFrameCodecDecoderTests.cpp
)

target_link_libraries(raw_frame_tests PRIVATE GTest::gtest_main EasyLoRa::protocol)

target_compile_features(raw_frame_tests PRIVATE cxx_std_23)

include(GoogleTest)

gtest_discover_tests(raw_frame_tests)

add_executable(protocol_codec_tests
	ProtocolCodecDecoderTests.cpp
)

target_link_libraries(protocol_codec_tests PRIVATE GTest::gtest_main EasyLoRa::protocol)

target_compile_features(protocol_codec_tests PRIVATE cxx_std_23)

gtest_discover_tests(protocol_codec_tests)
