function(project_set_warnings target)
    if(MSVC)
        target_compile_options(${target}
            PRIVATE
                /W4
                /permissive-
                /w14242
                /w14254
                /w14263
                /w14265
                /w14287
                /we4289
                /w14296
                /w14311
                /w14545
                /w14546
                /w14547
                /w14549
                /w14555
                /w14619
                /w14640
                /w14826
                /w14905
                /w14906
                /w14928
        )

        if(PROTOCOL_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wshadow
                -Wcast-align
                -Wcast-qual
                -Wdouble-promotion
                -Wformat=2
                -Wimplicit-fallthrough
                -Wnon-virtual-dtor
                -Wold-style-cast
                -Woverloaded-virtual
                -Wnull-dereference
                -Wundef
        )

        if(PROTOCOL_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
