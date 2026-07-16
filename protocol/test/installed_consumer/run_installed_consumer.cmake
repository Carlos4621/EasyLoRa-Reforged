if(NOT DEFINED INSTALLED_CONSUMER_SOURCE_DIR)
	message(FATAL_ERROR "INSTALLED_CONSUMER_SOURCE_DIR is required")
endif()

if(NOT DEFINED INSTALLED_CONSUMER_BUILD_DIR)
	message(FATAL_ERROR "INSTALLED_CONSUMER_BUILD_DIR is required")
endif()

if(NOT DEFINED PROTOCOL_BUILD_DIR)
	message(FATAL_ERROR "PROTOCOL_BUILD_DIR is required")
endif()

if(NOT DEFINED PROTOCOL_INSTALL_DIR)
	message(FATAL_ERROR "PROTOCOL_INSTALL_DIR is required")
endif()

file(REMOVE_RECURSE "${INSTALLED_CONSUMER_BUILD_DIR}" "${PROTOCOL_INSTALL_DIR}")

execute_process(
	COMMAND "${CMAKE_COMMAND}" --install "${PROTOCOL_BUILD_DIR}" --prefix "${PROTOCOL_INSTALL_DIR}" --component EasyLoRaProtocol
	RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
	message(FATAL_ERROR "Protocol install failed with ${install_result}")
endif()

execute_process(
	COMMAND "${CMAKE_COMMAND}"
		-S "${INSTALLED_CONSUMER_SOURCE_DIR}"
		-B "${INSTALLED_CONSUMER_BUILD_DIR}"
		-DCMAKE_PREFIX_PATH=${PROTOCOL_INSTALL_DIR}
	RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
	message(FATAL_ERROR "Installed consumer configure failed with ${configure_result}")
endif()

execute_process(
	COMMAND "${CMAKE_COMMAND}" --build "${INSTALLED_CONSUMER_BUILD_DIR}"
	RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
	message(FATAL_ERROR "Installed consumer build failed with ${build_result}")
endif()

execute_process(
	COMMAND "${INSTALLED_CONSUMER_BUILD_DIR}/installed_consumer"
	RESULT_VARIABLE run_result
)
if(NOT run_result EQUAL 0)
	message(FATAL_ERROR "Installed consumer executable failed with ${run_result}")
endif()
