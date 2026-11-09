cmake_minimum_required(VERSION 3.28...4.4)

project(keyinjectord
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../cmake/CompilerFlags.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/CompilerFlags.cmake")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/cmake/CompilerFlags.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/CompilerFlags.cmake")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
pkg_check_modules(LIBCAP REQUIRED IMPORTED_TARGET libcap)

add_executable(keyinjectord
    main.cpp
    capability.cpp
    uinput_device.cpp
    KeyboardMacroInjector.cpp
    ipc_server.cpp
    launcher_auth.cpp
    SocketCredentials.cpp
    ProcfsUtils.cpp
    FsSecurityChecker.cpp
    launcher_auth.h
    SocketCredentials.h
    ProcfsUtils.h
    FsSecurityChecker.h
    KeyboardMacroInjector.h
    device_interface.h
    logging.h
    protocol.h
)

target_link_libraries(keyinjectord PRIVATE
    PkgConfig::LIBEVDEV
    PkgConfig::LIBCAP
    qtscribe::compiler_flags
)

option(KEYINJECTORD_DEV_AUTH "Enable relaxed launcher authorization for development/testing" OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR KEYINJECTORD_DEV_AUTH)
    target_compile_definitions(keyinjectord PRIVATE KEYINJECTORD_DEV_AUTH=1)
endif()

if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    include(GNUInstallDirs)
    install(TARGETS keyinjectord
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
