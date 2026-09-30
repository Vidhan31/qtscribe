if(TARGET qtscribe_compiler_flags)
    return()
endif()

include(CheckCompilerFlag)
include(CheckLinkerFlag)

add_library(qtscribe_compiler_flags INTERFACE)
add_library(qtscribe::compiler_flags ALIAS qtscribe_compiler_flags)

target_compile_features(qtscribe_compiler_flags INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Base compiler warning and optimization flags
    target_compile_options(qtscribe_compiler_flags INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
        $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
        -Wformat=2
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-ffunction-sections>
        $<$<CONFIG:Release>:-fdata-sections>
        $<$<CONFIG:Release>:-fno-semantic-interposition>
        $<$<CONFIG:Release>:-U_FORTIFY_SOURCE>
        $<$<CONFIG:Release>:-D_FORTIFY_SOURCE=3>
    )

    # OpenSSF: Format security check
    check_compiler_flag(CXX "-Wformat-security" QTSCRIBE_HAS_WFORMAT_SECURITY)
    if(QTSCRIBE_HAS_WFORMAT_SECURITY)
        target_compile_options(qtscribe_compiler_flags INTERFACE -Wformat-security)
    endif()

    # OpenSSF: Stack protector strong
    check_compiler_flag(CXX "-fstack-protector-strong" QTSCRIBE_HAS_STACK_PROTECTOR_STRONG)
    if(QTSCRIBE_HAS_STACK_PROTECTOR_STRONG)
        target_compile_options(qtscribe_compiler_flags INTERFACE -fstack-protector-strong)
    endif()

    # OpenSSF: Position Independent Executables (PIE).
    # Only -pie at link time. Never add -fPIE here: on current GCC -fPIE
    # enables direct-extern-access codegen, emitting direct PC32 accesses
    # to Qt shared-library data (staticMetaObject, QString::_empty, ...)
    # instead of GOTPCREL. The (PIE) link then satisfies them with COPY
    # relocations that shadow the Qt definition, tripping NoQtCopyRelocTest
    # (and segfaulting against a protected-visibility Qt, e.g. with
    # QCoreApplication::self staying null during QGuiApplication startup).
    # Force -fPIC on every TU instead: CMake compiles executable targets
    # with -fPIE, but this option is appended after CMake's builtin flags
    # on the compile line, so -fPIC wins and GOT access is used everywhere.
    target_compile_options(qtscribe_compiler_flags INTERFACE -fPIC)
    check_linker_flag(CXX "-pie" QTSCRIBE_HAS_PIE)
    if(QTSCRIBE_HAS_PIE)
        target_link_options(qtscribe_compiler_flags INTERFACE -pie)
    endif()

    # OpenSSF: Relocation Read-Only (RELRO)
    check_linker_flag(CXX "LINKER:-z,relro" QTSCRIBE_HAS_RELRO)
    if(QTSCRIBE_HAS_RELRO)
        target_link_options(qtscribe_compiler_flags INTERFACE
            $<$<CONFIG:Release>:LINKER:-z,relro>
        )
    endif()

    # OpenSSF: Immediate binding / Full RELRO (BIND_NOW)
    check_linker_flag(CXX "LINKER:-z,now" QTSCRIBE_HAS_BIND_NOW)
    if(QTSCRIBE_HAS_BIND_NOW)
        target_link_options(qtscribe_compiler_flags INTERFACE
            $<$<CONFIG:Release>:LINKER:-z,now>
        )
    endif()

    # OpenSSF: Non-executable stack
    check_linker_flag(CXX "LINKER:-z,noexecstack" QTSCRIBE_HAS_NOEXECSTACK)
    if(QTSCRIBE_HAS_NOEXECSTACK)
        target_link_options(qtscribe_compiler_flags INTERFACE
            $<$<CONFIG:Release>:LINKER:-z,noexecstack>
        )
    endif()

    # Linker dead-code and dependency optimizations
    check_linker_flag(CXX "LINKER:--gc-sections" QTSCRIBE_HAS_GC_SECTIONS)
    if(QTSCRIBE_HAS_GC_SECTIONS)
        target_link_options(qtscribe_compiler_flags INTERFACE
            $<$<CONFIG:Release>:LINKER:--gc-sections>
        )
    endif()

    check_linker_flag(CXX "LINKER:--as-needed" QTSCRIBE_HAS_AS_NEEDED)
    if(QTSCRIBE_HAS_AS_NEEDED)
        target_link_options(qtscribe_compiler_flags INTERFACE
            $<$<CONFIG:Release>:LINKER:--as-needed>
        )
    endif()

    check_linker_flag(CXX "LINKER:-O1" QTSCRIBE_HAS_LINKER_O1)
    if(QTSCRIBE_HAS_LINKER_O1)
        target_link_options(qtscribe_compiler_flags INTERFACE
            $<$<CONFIG:Release>:LINKER:-O1>
        )
    endif()

    target_compile_definitions(qtscribe_compiler_flags INTERFACE
        $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
        $<$<CONFIG:Release>:QT_NO_INFO_OUTPUT>
        $<$<CONFIG:Release>:KEYINJECTORD_NO_DEBUG_OUTPUT>
        $<$<CONFIG:Release>:QT_USE_QSTRINGBUILDER>
    )
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "C compiler launcher" FORCE)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "CXX compiler launcher" FORCE)
    message(STATUS "Using ccache compiler launcher: ${CCACHE_PROGRAM}")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/Sanitizers.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Sanitizers.cmake")
endif()
