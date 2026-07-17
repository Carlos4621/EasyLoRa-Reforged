function(project_enable_static_analyzers target)
    if(PROTOCOL_ENABLE_CLANG_TIDY)
        find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)

        if(NOT CLANG_TIDY_EXECUTABLE)
            message(FATAL_ERROR "PROTOCOL_ENABLE_CLANG_TIDY está activo, pero clang-tidy no fue encontrado")
        endif()

        set_target_properties(${target}
            PROPERTIES
                CXX_CLANG_TIDY
                    "${CLANG_TIDY_EXECUTABLE};--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy"
        )
    endif()

    if(PROTOCOL_ENABLE_CPPCHECK)
        find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)

        if(NOT CPPCHECK_EXECUTABLE)
            message(FATAL_ERROR "PROTOCOL_ENABLE_CPPCHECK está activo, pero cppcheck no fue encontrado")
        endif()

        set_target_properties(${target}
            PROPERTIES
                CXX_CPPCHECK
                    "${CPPCHECK_EXECUTABLE};--enable=warning,performance,portability;--inline-suppr;--error-exitcode=1"
        )
    endif()
endfunction()
