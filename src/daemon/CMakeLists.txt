qt_add_executable(qtscribed
    main.cpp
)

target_compile_definitions(qtscribed
    PRIVATE
        QTSCRIBE_VERSION="${PROJECT_VERSION}"
)

target_include_directories(qtscribed
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(qtscribed
    PRIVATE
        qtscribe_core
        qtscribe::compiler_flags
)

install(TARGETS qtscribed
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
