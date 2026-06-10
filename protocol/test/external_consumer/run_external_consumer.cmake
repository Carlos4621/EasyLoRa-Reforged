if(NOT DEFINED EXTERNAL_CONSUMER_SOURCE_DIR)
	message(FATAL_ERROR "EXTERNAL_CONSUMER_SOURCE_DIR is required")
endif()

if(NOT DEFINED EXTERNAL_CONSUMER_BUILD_DIR)
	message(FATAL_ERROR "EXTERNAL_CONSUMER_BUILD_DIR is required")
endif()

if(NOT DEFINED PROTOCOL_SOURCE_DIR)
	message(FATAL_ERROR "PROTOCOL_SOURCE_DIR is required")
endif()

file(REMOVE_RECURSE "${EXTERNAL_CONSUMER_BUILD_DIR}")

execute_process(
	COMMAND "${CMAKE_COMMAND}"
		-S "${EXTERNAL_CONSUMER_SOURCE_DIR}"
		-B "${EXTERNAL_CONSUMER_BUILD_DIR}"
		-DPROTOCOL_SOURCE_DIR=${PROTOCOL_SOURCE_DIR}
	RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
	message(FATAL_ERROR "External consumer configure failed with ${configure_result}")
endif()

execute_process(
	COMMAND "${CMAKE_COMMAND}" --build "${EXTERNAL_CONSUMER_BUILD_DIR}"
	RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
	message(FATAL_ERROR "External consumer build failed with ${build_result}")
endif()

execute_process(
	COMMAND "${EXTERNAL_CONSUMER_BUILD_DIR}/external_consumer"
	RESULT_VARIABLE run_result
)
if(NOT run_result EQUAL 0)
	message(FATAL_ERROR "External consumer executable failed with ${run_result}")
endif()
