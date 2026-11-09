# cmake/Coverage.cmake
# Configures code coverage generation for first-party QtScribe targets.

if(TARGET qtscribe_coverage)
    return()
endif()

add_library(qtscribe_coverage INTERFACE)
add_library(qtscribe::coverage ALIAS qtscribe_coverage)

option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(NOT ENABLE_COVERAGE)
    return()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Enabling GCC code coverage (--coverage)")
    target_compile_options(qtscribe_coverage INTERFACE
        --coverage
        -fno-inline
        -fno-inline-small-functions
        -fno-default-inline
        -g
        -O0
    )
    target_link_options(qtscribe_coverage INTERFACE
        --coverage
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Enabling Clang code coverage (-fprofile-instr-generate -fcoverage-mapping)")
    target_compile_options(qtscribe_coverage INTERFACE
        -fprofile-instr-generate
        -fcoverage-mapping
        -fno-inline
        -g
        -O0
    )
    target_link_options(qtscribe_coverage INTERFACE
        -fprofile-instr-generate
        -fcoverage-mapping
    )
else()
    message(WARNING "Coverage is not configured for compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if(TARGET qtscribe_compiler_flags)
    target_link_libraries(qtscribe_compiler_flags INTERFACE qtscribe_coverage)
endif()
