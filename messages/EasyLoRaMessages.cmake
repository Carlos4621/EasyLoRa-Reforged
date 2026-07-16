include_guard(GLOBAL)

set_property(GLOBAL PROPERTY EASYLORA_MESSAGES_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(easylora_generate_protobuf OUT_SOURCES OUT_HEADERS OUTPUT_DIR)
	get_property(MESSAGES_DIR GLOBAL PROPERTY EASYLORA_MESSAGES_DIR)
	find_package(Protobuf REQUIRED)
	if(NOT Protobuf_PROTOC_EXECUTABLE)
		message(FATAL_ERROR "protoc not found. Install protobuf-compiler or set Protobuf_ROOT.")
	endif()

	file(GLOB MESSAGE_PROTO_FILES CONFIGURE_DEPENDS "${MESSAGES_DIR}/*.proto")
	file(MAKE_DIRECTORY "${OUTPUT_DIR}")

	set(GENERATED_SOURCES "")
	set(GENERATED_HEADERS "")
	foreach(PROTO_FILE IN LISTS MESSAGE_PROTO_FILES)
		get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)
		set(GENERATED_SOURCE "${OUTPUT_DIR}/${PROTO_NAME}.pb.cc")
		set(GENERATED_HEADER "${OUTPUT_DIR}/${PROTO_NAME}.pb.h")
		add_custom_command(
			OUTPUT "${GENERATED_SOURCE}" "${GENERATED_HEADER}"
			COMMAND "${Protobuf_PROTOC_EXECUTABLE}"
				--proto_path=${MESSAGES_DIR}
				--cpp_out=${OUTPUT_DIR}
				"${PROTO_FILE}"
			DEPENDS "${PROTO_FILE}"
			COMMENT "Generating C++ Protobuf classes for ${PROTO_NAME}"
			VERBATIM
		)
		list(APPEND GENERATED_SOURCES "${GENERATED_SOURCE}")
		list(APPEND GENERATED_HEADERS "${GENERATED_HEADER}")
	endforeach()

	set(${OUT_SOURCES} "${GENERATED_SOURCES}" PARENT_SCOPE)
	set(${OUT_HEADERS} "${GENERATED_HEADERS}" PARENT_SCOPE)
endfunction()

function(easylora_generate_nanopb OUT_SOURCES OUT_HEADERS OUTPUT_DIR)
	get_property(MESSAGES_DIR GLOBAL PROPERTY EASYLORA_MESSAGES_DIR)
	if(NOT Protobuf_PROTOC_EXECUTABLE)
		find_program(Protobuf_PROTOC_EXECUTABLE NAMES protoc)
	endif()
	if(NOT Protobuf_PROTOC_EXECUTABLE)
		message(FATAL_ERROR "protoc not found. Install protobuf-compiler or set Protobuf_PROTOC_EXECUTABLE.")
	endif()

	if(NANOPB_ROOT)
		set(NANOPB_SOURCE_DIR "${NANOPB_ROOT}")
	elseif(DEFINED nanopb_SOURCE_DIR AND EXISTS "${nanopb_SOURCE_DIR}/generator/protoc-gen-nanopb")
		set(NANOPB_SOURCE_DIR "${nanopb_SOURCE_DIR}")
	else()
		include(FetchContent)
		FetchContent_Declare(
			nanopb
			GIT_REPOSITORY https://github.com/nanopb/nanopb.git
			GIT_TAG 0.4.8
		)
		FetchContent_MakeAvailable(nanopb)
		set(NANOPB_SOURCE_DIR "${nanopb_SOURCE_DIR}")
	endif()

	set(NANOPB_PLUGIN "${NANOPB_SOURCE_DIR}/generator/protoc-gen-nanopb")
	if(NOT EXISTS "${NANOPB_PLUGIN}")
		message(FATAL_ERROR "nanopb plugin not found at ${NANOPB_PLUGIN}")
	endif()

	file(GLOB MESSAGE_PROTO_FILES CONFIGURE_DEPENDS "${MESSAGES_DIR}/*.proto")
	file(MAKE_DIRECTORY "${OUTPUT_DIR}")

	set(GENERATED_SOURCES "")
	set(GENERATED_HEADERS "")
	foreach(PROTO_FILE IN LISTS MESSAGE_PROTO_FILES)
		get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)
		set(OPTIONS_FILE "${MESSAGES_DIR}/${PROTO_NAME}.options")
		set(GENERATOR_DEPENDS "${PROTO_FILE}")
		if(EXISTS "${OPTIONS_FILE}")
			list(APPEND GENERATOR_DEPENDS "${OPTIONS_FILE}")
		endif()

		set(GENERATED_SOURCE "${OUTPUT_DIR}/${PROTO_NAME}.pb.c")
		set(GENERATED_HEADER "${OUTPUT_DIR}/${PROTO_NAME}.pb.h")
		add_custom_command(
			OUTPUT "${GENERATED_SOURCE}" "${GENERATED_HEADER}"
			COMMAND "${Protobuf_PROTOC_EXECUTABLE}"
				--proto_path=${MESSAGES_DIR}
				--plugin=protoc-gen-nanopb=${NANOPB_PLUGIN}
				--nanopb_opt=-I${MESSAGES_DIR}
				--nanopb_out=${OUTPUT_DIR}
				"${PROTO_FILE}"
			DEPENDS ${GENERATOR_DEPENDS}
			COMMENT "Generating nanopb classes for ${PROTO_NAME}"
			VERBATIM
		)
		list(APPEND GENERATED_SOURCES "${GENERATED_SOURCE}")
		list(APPEND GENERATED_HEADERS "${GENERATED_HEADER}")
	endforeach()

	set(${OUT_SOURCES} "${GENERATED_SOURCES}" PARENT_SCOPE)
	set(${OUT_HEADERS} "${GENERATED_HEADERS}" PARENT_SCOPE)
	set(EASYLORA_NANOPB_SOURCE_DIR "${NANOPB_SOURCE_DIR}" PARENT_SCOPE)
endfunction()
