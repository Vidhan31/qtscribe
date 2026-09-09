# cmake/Sanitizers.cmake
# Configures AddressSanitizer (ASan), UndefinedBehaviorSanitizer (UBSan), and ThreadSanitizer (TSan)
# for first-party QtScribe targets.

if(TARGET qtscribe_sanitizers)
    return()
endif()

add_library(qtscribe_sanitizers INTERFACE)
add_library(qtscribe::sanitizers ALIAS qtscribe_sanitizers)

option(ENABLE_ASAN_UBSAN "Enable Address and Undefined Behavior Sanitizers (ASan + UBSan)" OFF)
option(ENABLE_TSAN "Enable Thread Sanitizer (TSan)" OFF)

if(ENABLE_ASAN_UBSAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ENABLE_ASAN_UBSAN and ENABLE_TSAN are mutually exclusive and cannot be enabled together.")
endif()

if(NOT ENABLE_ASAN_UBSAN AND NOT ENABLE_TSAN)
    return()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(ENABLE_ASAN_UBSAN)
        message(STATUS "Enabling AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan)")

        target_compile_options(qtscribe_sanitizers INTERFACE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -fno-sanitize-recover=all
            -g
        )

        target_link_options(qtscribe_sanitizers INTERFACE
            -fsanitize=address,undefined
        )
    elseif(ENABLE_TSAN)
        message(STATUS "Enabling ThreadSanitizer (TSan)")

        target_compile_options(qtscribe_sanitizers INTERFACE
            -fsanitize=thread
            -fno-omit-frame-pointer
            -g
        )

        target_link_options(qtscribe_sanitizers INTERFACE
            -fsanitize=thread
        )
    endif()
else()
    message(WARNING "Sanitizers are not configured for compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if(TARGET qtscribe_compiler_flags)
    target_link_libraries(qtscribe_compiler_flags INTERFACE qtscribe_sanitizers)
endif()
