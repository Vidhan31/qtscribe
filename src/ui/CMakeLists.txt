set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(qtscribe_ui STATIC)
qt_add_qml_module(qtscribe_ui
    URI QtScribe
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/QtScribe"
    IMPORTS
        QtQuick
        QtQuick.Controls
        QtQuick.Layouts
        QtQuick.Templates
    SOURCES
        ${CMAKE_SOURCE_DIR}/src/core/AudioRecorder.h
        ${CMAKE_SOURCE_DIR}/src/core/GlobalShortcutManager.h
        ${CMAKE_SOURCE_DIR}/src/core/HttpRequestRunner.h
        ${CMAKE_SOURCE_DIR}/src/core/AbstractSttClient.h
        ${CMAKE_SOURCE_DIR}/src/core/AbstractLlmClient.h
        ${CMAKE_SOURCE_DIR}/src/core/GroqSttClient.h
        ${CMAKE_SOURCE_DIR}/src/core/GeminiSttClient.h
        ${CMAKE_SOURCE_DIR}/src/core/CloudSttRouter.h
        ${CMAKE_SOURCE_DIR}/src/core/WhisperSttClient.h
        ${CMAKE_SOURCE_DIR}/src/core/ModelDownloader.h
        ${CMAKE_SOURCE_DIR}/src/core/WhisperModelManager.h
        ${CMAKE_SOURCE_DIR}/src/core/GroqLlmClient.h
        ${CMAKE_SOURCE_DIR}/src/core/CompositeSttEngine.h
        ${CMAKE_SOURCE_DIR}/src/core/DictationCoordinator.h
        ${CMAKE_SOURCE_DIR}/src/core/DictationPadModel.h
        ${CMAKE_SOURCE_DIR}/src/core/SystemHealthMonitor.h
        ${CMAKE_SOURCE_DIR}/src/core/AudioFeedbackPlayer.h
        ${CMAKE_SOURCE_DIR}/src/core/DaemonDiagnosticModel.h
        ${CMAKE_SOURCE_DIR}/src/core/TranscriptionModel.h
        ${CMAKE_SOURCE_DIR}/src/core/LicenseHelper.h
        ${CMAKE_SOURCE_DIR}/src/core/ColorUtils.h
        ${CMAKE_SOURCE_DIR}/src/core/NavigationModel.h
        ${CMAKE_SOURCE_DIR}/src/core/CloudProviderModel.h
        ${CMAKE_SOURCE_DIR}/src/core/LinuxNotificationService.h
    QML_FILES
        Main.qml
        SpeechPanel.qml
        SystemDiagnosticsColumn.qml
        RecordingControl.qml
        SystemStatusFooter.qml
        HistoryView.qml
        CreditsView.qml
        LicenseView.qml
        Theme.qml
        StateBadge.qml
        StatusBanner.qml
        ShortcutGuideDialog.qml
        ClipboardWarningDialog.qml
        controls/StyledButton.qml
        controls/StyledCard.qml
        controls/StyledComboBox.qml
        controls/StyledDivider.qml
        controls/StyledIcon.qml
        controls/StyledSlider.qml
        controls/StyledText.qml
        controls/StyledTextField.qml
        settings/PreferenceRow.qml
        settings/PreferenceSwitch.qml
        settings/EngineModeSwitcherCard.qml
        settings/ApiKeyConfigCard.qml
        settings/CustomVocabularyCard.qml
        settings/CloudSettingsPage.qml
        settings/providers/GroqProviderSettings.qml
        settings/providers/GeminiProviderSettings.qml
        settings/OfflineSettingsPage.qml
        settings/SystemSettingsPage.qml
)

qt_add_resources(qtscribe_ui "qtscribe_license"
    PREFIX "/qt/qml/QtScribe/license"
    BASE "${CMAKE_SOURCE_DIR}"
    FILES
        ${CMAKE_SOURCE_DIR}/LICENSE
)

qt_add_resources(qtscribe_ui "qtscribe_assets"
    PREFIX "/qt/qml/QtScribe/assets"
    BASE "${CMAKE_SOURCE_DIR}/assets"
    FILES
        ${CMAKE_SOURCE_DIR}/assets/chime_start.wav
        ${CMAKE_SOURCE_DIR}/assets/chime_stop.wav
        ${CMAKE_SOURCE_DIR}/assets/speech-to-text-16.png
        ${CMAKE_SOURCE_DIR}/assets/speech-to-text-24.png
        ${CMAKE_SOURCE_DIR}/assets/speech-to-text-32.png
        ${CMAKE_SOURCE_DIR}/assets/speech-to-text-64.png
        ${CMAKE_SOURCE_DIR}/assets/speech-to-text-128.png
        ${CMAKE_SOURCE_DIR}/assets/speech-to-text-256.png
        ${CMAKE_SOURCE_DIR}/assets/speech-to-text-512.png
        ${CMAKE_SOURCE_DIR}/assets/icons/mic.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/stop.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/spinner.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/keyboard.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/bolt.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/copy.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/trash.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/check.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/history.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/key.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/speech.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/sparkles.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/info.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/license.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/save.svg
        ${CMAKE_SOURCE_DIR}/assets/icons/refresh.svg
)

target_include_directories(qtscribe_ui
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(qtscribe_ui
    PRIVATE
        QTSCRIBE_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(qtscribe_ui
    PUBLIC
        qtscribe_core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickTemplates2
        Qt6::QuickEffects
    PRIVATE
        qtscribe::compiler_flags
)

target_precompile_headers(qtscribe_ui
    PRIVATE
        <QGuiApplication>
        <QQmlApplicationEngine>
        <QQuickItem>
        <QObject>
        <QString>
)
