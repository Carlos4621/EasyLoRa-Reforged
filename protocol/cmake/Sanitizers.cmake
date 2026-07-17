function(project_enable_sanitizers target)
    if(PROTOCOL_ENABLE_ASAN AND PROTOCOL_ENABLE_TSAN)
        message(FATAL_ERROR "ASan y TSan deben utilizarse en compilaciones separadas")
    endif()

    if(MSVC)
        if(PROTOCOL_ENABLE_ASAN)
            target_compile_options(${target} PRIVATE /fsanitize=address)
        endif()
        return()
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        return()
    endif()

    set(sanitizers "")

    if(PROTOCOL_ENABLE_ASAN)
        list(APPEND sanitizers address)
    endif()

    if(PROTOCOL_ENABLE_UBSAN)
        list(APPEND sanitizers undefined)
    endif()

    if(PROTOCOL_ENABLE_TSAN)
        list(APPEND sanitizers thread)
    endif()

    if(NOT sanitizers)
        return()
    endif()

    list(JOIN sanitizers "," sanitizer_flags)

    target_compile_options(${target}
        PRIVATE
            -fsanitize=${sanitizer_flags}
            -fno-omit-frame-pointer
            -fno-optimize-sibling-calls
    )

    target_link_options(${target}
        PRIVATE
            -fsanitize=${sanitizer_flags}
    )

    if(PROTOCOL_ENABLE_UBSAN)
        target_compile_options(${target} PRIVATE -fno-sanitize-recover=undefined)
        target_link_options(${target} PRIVATE -fno-sanitize-recover=undefined)
    endif()
endfunction()
