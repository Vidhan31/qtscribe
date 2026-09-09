pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe

ColumnLayout {
    id: root

    signal navigateRequested(string target)
    signal clipboardWarningRequested

    spacing: Theme.spacingMd

    StatusBanner {
        visible: DaemonDiagnosticModel.clipboardWarningRequired && !DaemonDiagnosticModel.clipboardBannerDismissed
        bannerType: "warning"
        title: qsTr("Paste Shortcut & Clipboard Notice")
        message: qsTr(
                     "Transcribing injects text via Ctrl+Shift+V. Text is restored, but images/media are overwritten unless saved or using a clipboard manager (e.g., Klipper, GPaste, Clipboard Indicator).")
        actionText: qsTr("Learn More")
        onActionClicked: root.clipboardWarningRequested()
        secondaryActionText: qsTr("Dismiss")
        onSecondaryActionClicked: {
            DaemonDiagnosticModel.clipboardBannerDismissed = true;
        }
    }

    StatusBanner {
        visible: DictationCoordinator.activeBackend === DictationCoordinator.Cloud && !CloudSttRouter.apiKeySet
        bannerType: "warning"
        title: qsTr("%1 API Key Required").arg(CloudProviderModel.activeProviderName)
        message: qsTr("Configure your %1 API key in Settings to begin cloud speech transcription.").arg(
                     CloudProviderModel.activeProviderName)
        actionText: qsTr("Configure API Key")
        onActionClicked: root.navigateRequested("cloud")
    }

    StatusBanner {
        visible: DictationCoordinator.activeBackend === DictationCoordinator.Cloud && CloudSttRouter.isApiKeyInvalid
        bannerType: "warning"
        title: qsTr("Invalid %1 API Key").arg(CloudProviderModel.activeProviderName)
        message: CloudSttRouter.lastError.length > 0 ? CloudSttRouter.lastError : qsTr(
                                                           "Authentication failed. Please verify your %1 API key.").arg(
                                                           CloudProviderModel.activeProviderName)
        actionText: qsTr("Configure API Key")
        onActionClicked: root.navigateRequested("cloud")
        secondaryActionText: qsTr("Dismiss")
        onSecondaryActionClicked: DictationCoordinator.clearError()
    }

    StatusBanner {
        visible: DictationCoordinator.activeBackend === DictationCoordinator.Cloud && CloudSttRouter.isRateLimited
        bannerType: "warning"
        title: qsTr("%1 Rate Limit Exceeded").arg(CloudProviderModel.activeProviderName)
        message: qsTr("Auto-retrying in %1s…").arg(CloudSttRouter.retrySecondsRemaining)
        actionText: qsTr("Dismiss")
        onActionClicked: DictationCoordinator.clearError()
    }

    StatusBanner {
        visible: DictationCoordinator.activeBackend === DictationCoordinator.WhisperCpp &&
                 !WhisperSttClient.isModelInstalled
        bannerType: "warning"
        title: qsTr("Offline Whisper Model Missing")
        message: qsTr("Download %1 to start offline transcription.").arg(WhisperSttClient.modelFileName)
        actionText: qsTr("Offline Settings")
        onActionClicked: root.navigateRequested("offline")
    }

    StatusBanner {
        visible: DictationCoordinator.activeBackend === DictationCoordinator.WhisperCpp
                 && WhisperSttClient.isModelInstalled && !WhisperSttClient.isModelLoaded
        bannerType: "info"
        title: qsTr("Loading Whisper Model")
        message: qsTr("Loading offline speech recognition model into memory…")
        actionText: qsTr("Offline Settings")
        onActionClicked: root.navigateRequested("offline")
    }

    StatusBanner {
        visible: !AudioRecorder.hasAudioInputDevice
        bannerType: "warning"
        title: qsTr("No Microphone Detected")
        message: qsTr("Please connect a microphone or check your audio permissions in system settings.")
        actionText: qsTr("Audio Settings")
        onActionClicked: root.navigateRequested("system")
    }

    StatusBanner {
        visible: DaemonDiagnosticModel.hasFatalError
        bannerType: "danger"
        title: qsTr("Direct Typing Service Stopped")
        message: DaemonDiagnosticModel.fatalErrorMessage.length > 0 ? DaemonDiagnosticModel.fatalErrorMessage : qsTr(
                                                                          "The background key injection daemon encountered an error. Text will fallback to clipboard paste until restarted.")
        actionText: qsTr("Restart Service")
        onActionClicked: DaemonDiagnosticModel.restartService()
        secondaryActionText: qsTr("Settings")
        onSecondaryActionClicked: root.navigateRequested("system")
    }

    StatusBanner {
        visible: DictationCoordinator.dictationState === DictationCoordinator.Error && (
                     DictationCoordinator.activeBackend !== DictationCoordinator.Cloud || (
                         !CloudSttRouter.isApiKeyInvalid && !CloudSttRouter.isRateLimited))
        bannerType: "danger"
        title: DictationCoordinator.activeBackend === DictationCoordinator.WhisperCpp ? qsTr(
                                                                                            "Offline Transcription Failed") :
                                                                                        qsTr("Transcription Failed")
        message: DictationCoordinator.lastError.length > 0 ? DictationCoordinator.lastError : qsTr(
                                                                 "An error occurred during transcription.")
        actionText: qsTr("Retry Transcription")
        onActionClicked: DictationCoordinator.retryTranscription()
        secondaryActionText: qsTr("Dismiss")
        onSecondaryActionClicked: DictationCoordinator.clearError()
    }
}
