find_package(Qt6 REQUIRED COMPONENTS Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCAP REQUIRED IMPORTED_TARGET libcap)

set(QT_TEST_LIB_DIRS "")
foreach(PREFIX_ENTRY IN LISTS CMAKE_PREFIX_PATH)
    file(REAL_PATH "${PREFIX_ENTRY}/lib" EXPANDED_LIB_DIR EXPAND_TILDE)
    if(EXISTS "${EXPANDED_LIB_DIR}")
        list(APPEND QT_TEST_LIB_DIRS "${EXPANDED_LIB_DIR}")
    endif()
endforeach()
if(DEFINED QT6_INSTALL_LIBS AND EXISTS "${QT6_INSTALL_LIBS}")
    list(APPEND QT_TEST_LIB_DIRS "${QT6_INSTALL_LIBS}")
endif()
if(QT_TEST_LIB_DIRS)
    list(REMOVE_DUPLICATES QT_TEST_LIB_DIRS)
    list(JOIN QT_TEST_LIB_DIRS ":" QT_TEST_LIB_PATH_STR)
    set(QT_TEST_LD_PREPEND "${QT_TEST_LIB_PATH_STR}:")
else()
    set(QT_TEST_LD_PREPEND "")
endif()

set(QTSCRIBE_TEST_ENVIRONMENT
    "QT_QPA_PLATFORM=offscreen"
    "QT_MEDIA_BACKEND=null"
    "LD_LIBRARY_PATH=${QT_TEST_LD_PREPEND}$ENV{LD_LIBRARY_PATH}"
)

if(ENABLE_ASAN_UBSAN)
    list(APPEND QTSCRIBE_TEST_ENVIRONMENT
        "ASAN_OPTIONS=detect_leaks=1:halt_on_error=1"
        "LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/tests/sanitizers/lsan.supp"
        "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1:suppressions=${CMAKE_SOURCE_DIR}/tests/sanitizers/ubsan.supp"
    )
elseif(ENABLE_TSAN)
    list(APPEND QTSCRIBE_TEST_ENVIRONMENT
        "TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1:suppressions=${CMAKE_SOURCE_DIR}/tests/sanitizers/tsan.supp"
    )
endif()

function(add_qtscribe_test name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "TEST_NAME" "SOURCES;LIBRARIES;DEFINITIONS;INCLUDES")

    set(target "test_${name}")
    if(ARG_TEST_NAME)
        set(ctest_name "${ARG_TEST_NAME}")
    else()
        set(ctest_name "${name}Test")
    endif()

    if(NOT ARG_SOURCES)
        set(ARG_SOURCES "test_${name}.cpp")
    endif()

    qt_add_executable(${target} ${ARG_SOURCES})

    target_include_directories(${target}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/core
            ${ARG_INCLUDES}
    )

    target_link_libraries(${target}
        PRIVATE
            qtscribe_core
            qtscribe::compiler_flags
            Qt6::Core
            Qt6::Test
            ${ARG_LIBRARIES}
    )

    if(ARG_DEFINITIONS)
        target_compile_definitions(${target} PRIVATE ${ARG_DEFINITIONS})
    endif()

    set_target_properties(${target} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION FALSE
    )

    add_test(NAME ${ctest_name} COMMAND ${target})
    set_tests_properties(${ctest_name} PROPERTIES
        ENVIRONMENT "${QTSCRIBE_TEST_ENVIRONMENT}"
    )
endfunction()

add_qtscribe_test(whisper_pipeline
    TEST_NAME WhisperPipelineTest
    LIBRARIES Qt6::Gui Qt6::Multimedia Qt6::Network
)

add_qtscribe_test(whisper_model_manager
    TEST_NAME WhisperModelManagerTest
    LIBRARIES Qt6::Network
)

add_qtscribe_test(dictation_coordinator
    TEST_NAME DictationCoordinatorTest
    LIBRARIES Qt6::Gui Qt6::Multimedia Qt6::Network Qt6::DBus
)

add_qtscribe_test(http_request_runner
    TEST_NAME HttpRequestRunnerTest
    LIBRARIES Qt6::Network
)

add_qtscribe_test(ipc_server
    TEST_NAME IpcServerTest
    SOURCES
        test_ipc_server.cpp
        ${CMAKE_SOURCE_DIR}/src/keyinjectord/ipc_server.cpp
        ${CMAKE_SOURCE_DIR}/src/keyinjectord/KeyboardMacroInjector.cpp
        ${CMAKE_SOURCE_DIR}/src/keyinjectord/launcher_auth.cpp
        ${CMAKE_SOURCE_DIR}/src/keyinjectord/SocketCredentials.cpp
        ${CMAKE_SOURCE_DIR}/src/keyinjectord/ProcfsUtils.cpp
        ${CMAKE_SOURCE_DIR}/src/keyinjectord/FsSecurityChecker.cpp
    INCLUDES
        ${CMAKE_SOURCE_DIR}/src/keyinjectord
    LIBRARIES
        Qt6::Network
        PkgConfig::LIBCAP
    DEFINITIONS
        KEYINJECTORD_DEV_AUTH=1
)

add_qtscribe_test(groq_stt_client
    TEST_NAME GroqSttClientTest
    LIBRARIES Qt6::Network
)

add_qtscribe_test(network_tls
    TEST_NAME NetworkTlsTest
    LIBRARIES Qt6::Network
)

add_qtscribe_test(status_notifier
    TEST_NAME StatusNotifierTest
    LIBRARIES Qt6::Gui Qt6::Network Qt6::DBus
)

add_qtscribe_test(application_context
    TEST_NAME ApplicationContextTest
    LIBRARIES Qt6::Gui Qt6::Multimedia Qt6::Network Qt6::DBus
)

add_qtscribe_test(cloud_provider_model
    TEST_NAME CloudProviderModelTest
)

add_qtscribe_test(gemini_stt_client
    TEST_NAME GeminiSttClientTest
    LIBRARIES Qt6::Network
)

add_qtscribe_test(cloud_stt_router
    TEST_NAME CloudSttRouterTest
    LIBRARIES Qt6::Network
)

add_qtscribe_test(linux_notification_service
    TEST_NAME LinuxNotificationServiceTest
    LIBRARIES Qt6::Gui Qt6::DBus
)

add_qtscribe_test(text_injector
    TEST_NAME TextInjectorTest
    LIBRARIES Qt6::Network
)

add_qtscribe_test(daemon_diagnostic_model
    TEST_NAME DaemonDiagnosticModelTest
    LIBRARIES Qt6::Network
)

add_qtscribe_test(composite_stt_engine
    TEST_NAME CompositeSttEngineTest
    LIBRARIES Qt6::Network Qt6::DBus
)

add_qtscribe_test(notification_presenter
    TEST_NAME NotificationPresenterTest
    LIBRARIES Qt6::Gui Qt6::Multimedia Qt6::Network Qt6::DBus
)

add_test(NAME WaylandOnlyPackagingTest
    COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/ci/check-wayland-only.sh"
)
set_tests_properties(WaylandOnlyPackagingTest PROPERTIES
    ENVIRONMENT "${QTSCRIBE_TEST_ENVIRONMENT}"
)

add_test(NAME NoQtCopyRelocTest
    COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/ci/check-no-qt-copy-reloc.sh" "$<TARGET_FILE:qtscribe>"
)
set_tests_properties(NoQtCopyRelocTest PROPERTIES
    ENVIRONMENT "${QTSCRIBE_TEST_ENVIRONMENT}"
)
