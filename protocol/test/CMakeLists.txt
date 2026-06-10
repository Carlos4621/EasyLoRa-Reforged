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

if(EASYLORA_ENABLE_NANOPB AND TARGET protobuf-nanopb-static)
	add_executable(nanopb_compatibility_tests
		NanopbCompatibilityTests.cpp
		"${NANOPB_GEN_DIR}/DeviceInfo.pb.c"
	)

	add_dependencies(nanopb_compatibility_tests generate_protocol_code)
	target_include_directories(nanopb_compatibility_tests BEFORE PRIVATE "${NANOPB_GEN_DIR}")
	target_link_libraries(nanopb_compatibility_tests PRIVATE GTest::gtest_main EasyLoRa::protocol protobuf-nanopb-static)
	target_compile_features(nanopb_compatibility_tests PRIVATE cxx_std_23)

	gtest_discover_tests(nanopb_compatibility_tests)
endif()

add_test(
	NAME ExternalConsumer.AddSubdirectoryLinksPublicTarget
	COMMAND "${CMAKE_COMMAND}"
		-DEXTERNAL_CONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/external_consumer
		-DEXTERNAL_CONSUMER_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/external_consumer_build
		-DPROTOCOL_SOURCE_DIR=${CMAKE_SOURCE_DIR}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/external_consumer/run_external_consumer.cmake
)
