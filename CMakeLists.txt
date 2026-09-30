cmake_minimum_required(VERSION 3.28...4.4)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure-time security guards
set(CMAKE_TLS_VERIFY ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prefix path resolution: strip empty entries and fall back to QT_DIR, CMAKE_PREFIX_PATH, or ~/Qt/6.11.1/gcc_64
if(DEFINED CMAKE_PREFIX_PATH)
    list(REMOVE_ITEM CMAKE_PREFIX_PATH "")
endif()
if(NOT CMAKE_PREFIX_PATH)
    if(DEFINED ENV{QT_DIR} AND NOT "$ENV{QT_DIR}" STREQUAL "")
        list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
    elseif(DEFINED ENV{CMAKE_PREFIX_PATH} AND NOT "$ENV{CMAKE_PREFIX_PATH}" STREQUAL "")
        list(APPEND CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")
    elseif(EXISTS "$ENV{HOME}/Qt/6.11.1/gcc_64")
        list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.11.1/gcc_64")
    endif()
endif()

if(NOT DEFINED QTSCRIBE_VERSION)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" QTSCRIBE_VERSION_FILE)
        string(STRIP "${QTSCRIBE_VERSION_FILE}" QTSCRIBE_VERSION)
    else()
        set(QTSCRIBE_VERSION "1.0.0")
    endif()
endif()

project(QtScribe VERSION ${QTSCRIBE_VERSION} LANGUAGES C CXX)

option(ENABLE_LTO "Enable Interprocedural / Link-Time Optimization (LTO) for release executables" OFF)
option(QTSCRIBE_WERROR "Treat compiler warnings as errors for QtScribe targets" OFF)

set_property(GLOBAL PROPERTY JOB_POOLS link_pool=2)
set(CMAKE_JOB_POOL_LINK link_pool)

set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib:$ORIGIN/lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

find_package(Qt6 6.11 REQUIRED COMPONENTS Gui Qml Quick QuickControls2 Network Multimedia DBus WaylandClient QuickEffects)
find_package(Qt6Keychain REQUIRED)

qt_standard_project_setup(REQUIRES 6.11)

option(ENABLE_QT_DEPLOYMENT "Enable Qt deployment script generation for standalone bundle packaging" ON)
option(BUILD_KEYINJECTORD "Build keyinjectord helper daemon" ON)
option(ENABLE_CPACK_DEB "Configure CPack for Debian (.deb) package generation" OFF)

include(cmake/CompilerFlags.cmake)
include(FetchContent)

find_package(Vulkan QUIET COMPONENTS glslc)
find_package(SPIRV-Headers QUIET CONFIG)
if(Vulkan_FOUND AND Vulkan_glslc_FOUND AND SPIRV-Headers_FOUND)
    message(STATUS "Vulkan SDK, glslc, and SPIRV-Headers found (${Vulkan_VERSION}): enabling Vulkan acceleration for whisper.cpp")
    set(GGML_VULKAN ON CACHE BOOL "whisper: enable Vulkan backend" FORCE)
    add_compile_definitions(GGML_USE_VULKAN)
else()
    message(STATUS "Vulkan SDK, glslc, or SPIRV-Headers not found: building whisper.cpp with CPU inference only")
    set(GGML_VULKAN OFF CACHE BOOL "whisper: enable Vulkan backend" FORCE)
endif()

set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "whisper: disable examples" FORCE)
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "whisper: disable tests" FORCE)
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "whisper: disable server" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "whisper: build static libraries" FORCE)
set(GGML_CCACHE ON CACHE BOOL "ggml: use ccache" FORCE)

# Fetch whisper.cpp for embedded local speech-to-text inference.
# For offline or air-gapped CI environments, pre-clone the repository and configure with:
#   -DFETCHCONTENT_SOURCE_DIR_WHISPER=/opt/whisper-src
#   -DFETCHCONTENT_FULLY_DISCONNECTED=ON
FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggml-org/whisper.cpp.git
    GIT_TAG        371b5a7561823ab2bb32142d2751e35e7534727b # v1.9.3
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
    FIND_PACKAGE_ARGS NAMES whisper whisper.cpp
)
FetchContent_MakeAvailable(whisper)

# Keep third-party whisper and ggml targets free of warnings-as-errors and clang-tidy
foreach(tgt whisper ggml ggml-base ggml-cpu ggml-vulkan)
    if(TARGET ${tgt})
        set_target_properties(${tgt} PROPERTIES
            COMPILE_WARNING_AS_ERROR OFF
            CXX_CLANG_TIDY ""
            C_CLANG_TIDY ""
        )
    endif()
endforeach()

option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis on first-party targets" OFF)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-19)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        message(STATUS "clang-tidy static analysis enabled: ${CLANG_TIDY_EXE}")
    else()
        message(FATAL_ERROR "ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
    endif()
endif()

# Enforce warnings-as-errors on first-party QtScribe targets when requested
if(QTSCRIBE_WERROR)
    set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
endif()

add_subdirectory(src/core)
add_subdirectory(src/ui)
if(BUILD_KEYINJECTORD)
    add_subdirectory(src/keyinjectord)
endif()

qt_add_executable(qtscribe
    src/main.cpp
)

target_compile_definitions(qtscribe
    PRIVATE
        QTSCRIBE_VERSION="${PROJECT_VERSION}"
)

target_include_directories(qtscribe
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(qtscribe
    PRIVATE
        qtscribe_core
        qtscribe_ui
        qtscribe_uiplugin
        qtscribe::compiler_flags
)

target_precompile_headers(qtscribe
    PRIVATE
        <QGuiApplication>
        <QQmlApplicationEngine>
        <QIcon>
)

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set_target_properties(qtscribe PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    endif()
endif()

find_program(CLANG_FORMAT_BIN
    NAMES clang-format clang-format-22 clang-format-21 clang-format-20 clang-format-19 clang-format-18 clang-format-17
)

find_program(QMLFORMAT_BIN
    NAMES qmlformat qmlformat-qt6
    HINTS
        ${QT6_INSTALL_BINS}
        ${_qt6_install_prefix}/bin
        /usr/lib64/qt6/bin
        /usr/lib/qt6/bin
)

if(CLANG_FORMAT_BIN)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    )
    add_custom_target(format-cpp
        COMMAND ${CLANG_FORMAT_BIN} -i -style=file ${ALL_CXX_SOURCE_FILES}
        COMMENT "Formatting C++ source files with clang-format"
    )
    add_custom_target(format-check-cpp
        COMMAND ${CLANG_FORMAT_BIN} --dry-run --Werror -style=file ${ALL_CXX_SOURCE_FILES}
        COMMENT "Checking C++ source file formatting with clang-format"
    )
endif()

if(QMLFORMAT_BIN)
    file(GLOB_RECURSE ALL_QML_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.qml
    )
    add_custom_target(format-qml
        COMMAND ${QMLFORMAT_BIN} --inplace ${ALL_QML_SOURCE_FILES}
        COMMENT "Formatting QML source files with qmlformat"
    )
endif()

if(TARGET format-cpp AND TARGET format-qml)
    add_custom_target(format
        DEPENDS format-cpp format-qml
        COMMENT "Formatting all C++ and QML files"
    )
elseif(TARGET format-cpp)
    add_custom_target(format DEPENDS format-cpp)
elseif(TARGET format-qml)
    add_custom_target(format DEPENDS format-qml)
endif()

include(GNUInstallDirs)

install(TARGETS qtscribe
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(TARGET keyinjectord)
    install(TARGETS keyinjectord
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    add_dependencies(qtscribe keyinjectord)
endif()

if(ENABLE_CPACK_DEB)
    set(DESKTOP_AND_ICON_PREFIX "/usr/share")
else()
    set(DESKTOP_AND_ICON_PREFIX "${CMAKE_INSTALL_DATADIR}")
endif()

install(
    FILES assets/io.github.qtscribe.desktop
    DESTINATION ${DESKTOP_AND_ICON_PREFIX}/applications
)

set(APP_ICON_SIZES 16 24 32 64 128 256 512)
foreach(SIZE ${APP_ICON_SIZES})
    install(FILES "assets/speech-to-text-${SIZE}.png"
        DESTINATION "${DESKTOP_AND_ICON_PREFIX}/icons/hicolor/${SIZE}x${SIZE}/apps"
        RENAME "io.github.qtscribe.png"
    )
    install(FILES "assets/speech-to-text-${SIZE}.png"
        DESTINATION "${DESKTOP_AND_ICON_PREFIX}/icons/hicolor/${SIZE}x${SIZE}/apps"
        RENAME "qtscribe.png"
    )
endforeach()

if(ENABLE_QT_DEPLOYMENT)
    qt_generate_deploy_qml_app_script(
        TARGET qtscribe
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS

        INCLUDE_PLUGIN_TYPES
            wayland-graphics-integration-client
            wayland-shell-integration
            wayland-decoration-client
            iconengines
            imageformats
            multimedia
            platformthemes
            platforminputcontexts

        INCLUDE_PLUGINS
            qwayland
            qwayland-generic

        EXCLUDE_PLUGIN_TYPES
            xcbglintegrations
            egldeviceintegrations

        EXCLUDE_PLUGINS
            qxcb
            qeglfs
            qlinuxfb
            qvnc
            qvkkhrdisplay
            qminimalegl

        POST_INCLUDE_REGEXES
            ".*libQt6.*\\.so.*"
            ".*libqt6keychain.*\\.so.*"
            ".*libicu.*\\.so.*"
            ".*libq.*\\.so.*"
        POST_EXCLUDE_REGEXES
            ".*libX11.*"
            ".*libxcb.*"
            ".*libXau.*"
            ".*libXdmcp.*"
            ".*"
    )
    install(SCRIPT ${deploy_script})

    if(ENABLE_CPACK_DEB)
        include(packaging/deb/CPackDebConfig.cmake)
        include(CPack)
    endif()
endif()

include(CTest)
enable_testing()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
