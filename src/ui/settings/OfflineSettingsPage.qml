pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import ".."
import "../controls"

Item {
    id: root

    implicitWidth: 500
    implicitHeight: contentColumn.implicitHeight

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Theme.spacingLg

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingXs
            Layout.bottomMargin: Theme.spacingSm
            spacing: 4

            StyledText {
                text: qsTr("Offline Dictation")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Configure on-device whisper.cpp speech recognition models and hardware acceleration")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        EngineModeSwitcherCard {
            Layout.fillWidth: true
        }

        StyledCard {
            title: qsTr("Active Whisper Model")
            description: qsTr("Current model selected for offline transcription inference")

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMd

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    RowLayout {
                        spacing: Theme.spacingSm

                        StyledText {
                            text: WhisperModelManager.isSelectedModelInstalled ? WhisperModelManager.selectedModelName :
                                                                                 qsTr("None Selected")
                            variant: "body"
                            customWeight: Font.DemiBold
                        }

                        StyledText {
                            text: "(" + WhisperSttClient.modelFileName + ")"
                            variant: "caption"
                            colorRole: "secondary"
                            visible: WhisperModelManager.isSelectedModelInstalled
                        }
                    }

                    StyledText {
                        text: {
                            if (WhisperSttClient.isModelLoaded && WhisperSttClient.loadedModelPath
                                === WhisperSttClient.modelPath) {
                                return qsTr("Loaded into memory and ready for dictation");
                            }
                            if (WhisperModelManager.isSelectedModelInstalled) {
                                return qsTr("Installed on disk (click Load to activate in memory)");
                            }
                            return qsTr("No active model installed. Download a model below to begin.");
                        }
                        variant: "caption"
                        colorRole: "secondary"
                    }
                }

                StateBadge {
                    text: {
                        if (WhisperSttClient.isModelLoaded && WhisperSttClient.loadedModelPath
                            === WhisperSttClient.modelPath) {
                            return qsTr("Ready");
                        }
                        if (WhisperModelManager.isSelectedModelInstalled) {
                            return qsTr("Installed");
                        }
                        return qsTr("No Model");
                    }
                    statusType: {
                        if (WhisperSttClient.isModelLoaded && WhisperSttClient.loadedModelPath
                            === WhisperSttClient.modelPath) {
                            return "success";
                        }
                        if (WhisperModelManager.isSelectedModelInstalled) {
                            return "neutral";
                        }
                        return "neutral";
                    }
                    showDot: WhisperModelManager.isSelectedModelInstalled
                    pulsing: WhisperSttClient.busy
                }
            }

            StyledDivider {}

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                StyledButton {
                    text: (WhisperSttClient.isModelLoaded && WhisperSttClient.loadedModelPath
                           === WhisperSttClient.modelPath) ? qsTr("Reload Model") : qsTr("Load Model")
                    size: "small"
                    variant: (WhisperSttClient.isModelLoaded && WhisperSttClient.loadedModelPath
                              === WhisperSttClient.modelPath) ? "secondary" : "primary"
                    enabled: WhisperModelManager.isSelectedModelInstalled && !WhisperSttClient.busy
                    onClicked: {
                        WhisperSttClient.loadModel();
                    }
                }

                StyledButton {
                    text: qsTr("Unload Model")
                    size: "small"
                    variant: "secondary"
                    visible: WhisperSttClient.isModelLoaded
                    enabled: !WhisperSttClient.busy
                    onClicked: {
                        WhisperSttClient.unloadModel();
                    }
                }

                StyledButton {
                    text: qsTr("Check Status")
                    size: "small"
                    variant: "flat"
                    onClicked: {
                        WhisperModelManager.refreshModelList();
                        WhisperSttClient.checkModelStatus();
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        StyledCard {
            title: qsTr("Whisper Model Library")
            description: qsTr("Download and manage speech recognition models from HuggingFace (ggerganov/whisper.cpp)")

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: storageHeaderRow.implicitHeight + Theme.spacingSm * 2
                radius: Theme.radiusSm
                color: Theme.controlBg
                border.color: Theme.controlBorder
                border.width: 1

                RowLayout {
                    id: storageHeaderRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: Theme.spacingSm
                    spacing: Theme.spacingSm

                    StyledIcon {
                        source: "qrc:/qt/qml/QtScribe/assets/icons/info.svg"
                        size: 14
                        color: Theme.textSecondary
                        Layout.alignment: Qt.AlignVCenter
                    }

                    StyledText {
                        text: qsTr("Storage:")
                        variant: "caption"
                        customWeight: Font.Medium
                    }

                    StyledText {
                        text: WhisperModelManager.modelsDirectory
                        variant: "caption"
                        colorRole: "secondary"
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        implicitWidth: 1
                        implicitHeight: 12
                        color: Theme.controlBorder
                    }

                    StyledText {
                        text: qsTr("Free: %1").arg(WhisperModelManager.availableDiskSpaceFormatted)
                        variant: "caption"
                        customWeight: Font.Medium
                        colorRole: "secondary"
                    }

                    StyledButton {
                        text: qsTr("Open")
                        size: "small"
                        variant: "ghost"
                        onClicked: {
                            Qt.openUrlExternally("file://" + WhisperModelManager.modelsDirectory);
                        }
                    }

                    StyledButton {
                        text: qsTr("Refresh")
                        iconSource: "qrc:/qt/qml/QtScribe/assets/icons/refresh.svg"
                        size: "small"
                        variant: "flat"
                        onClicked: {
                            WhisperModelManager.refreshModelList();
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: errorLayout.implicitHeight + Theme.spacingSm * 2
                radius: Theme.radiusSm
                color: Theme.statusDangerBg
                border.color: Theme.statusDangerBorder
                border.width: 1
                visible: WhisperModelManager.lastError.length > 0

                RowLayout {
                    id: errorLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: Theme.spacingSm
                    spacing: Theme.spacingSm

                    StyledText {
                        text: WhisperModelManager.lastError
                        variant: "caption"
                        customColor: Theme.colorDanger
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    StyledButton {
                        text: qsTr("Dismiss")
                        size: "small"
                        variant: "flat"
                        onClicked: {
                            WhisperModelManager.checkDiskSpace();
                        }
                    }
                }
            }

            StyledDivider {}

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                Repeater {
                    model: WhisperModelManager

                    delegate: Rectangle {
                        id: modelRow
                        required property int index
                        required property string modelId
                        required property string name
                        required property string fileName
                        required property string downloadUrl
                        required property string sizeFormatted
                        required property string memoryFormatted
                        required property string description
                        required property bool isInstalled
                        required property bool isSelected
                        required property bool isDownloading
                        required property real progress
                        required property string speedFormatted
                        required property string installedSizeFormatted
                        required property bool canDelete

                        Layout.fillWidth: true
                        implicitHeight: rowContent.implicitHeight + Theme.spacingSm * 2 + (modelRow.isDownloading
                                                                                           ? Theme.spacingXs : 0)
                        radius: Theme.radiusSm
                        color: modelRow.isSelected ? Theme.selectedBg : Theme.cardBgSubtle
                        border.color: modelRow.isSelected ? Theme.accentColor : Theme.cardBorder
                        border.width: 1

                        ColumnLayout {
                            id: rowContent
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: Theme.spacingSm
                            spacing: Theme.spacingXs

                            GridLayout {
                                Layout.fillWidth: true
                                columns: root.width >= 540 ? 3 : 1
                                columnSpacing: Theme.spacingSm
                                rowSpacing: Theme.spacingXs

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.spacingSm

                                        StyledText {
                                            text: modelRow.name
                                            variant: "body"
                                            customWeight: Font.DemiBold
                                        }

                                        StyledText {
                                            text: modelRow.isInstalled && modelRow.installedSizeFormatted.length > 0
                                                  ? modelRow.installedSizeFormatted : modelRow.sizeFormatted
                                            variant: "caption"
                                            colorRole: "secondary"
                                        }

                                        Rectangle {
                                            implicitHeight: 18
                                            implicitWidth: memText.implicitWidth + 8
                                            radius: Theme.radiusXs
                                            color: Theme.controlBg
                                            border.color: Theme.controlBorder
                                            border.width: 1

                                            StyledText {
                                                id: memText
                                                anchors.centerIn: parent
                                                text: modelRow.memoryFormatted
                                                variant: "caption"
                                                customWeight: Font.Medium
                                                colorRole: "secondary"
                                            }
                                        }

                                        StyledText {
                                            text: "(" + modelRow.fileName + ")"
                                            variant: "caption"
                                            colorRole: "secondary"
                                            elide: Text.ElideRight
                                            Layout.maximumWidth: 150
                                            visible: root.width >= 480
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }
                                    }

                                    StyledText {
                                        text: modelRow.description
                                        variant: "caption"
                                        colorRole: "secondary"
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                    }
                                }

                                StateBadge {
                                    text: qsTr("Active")
                                    statusType: "accent"
                                    visible: modelRow.isSelected
                                    Layout.alignment: root.width >= 540 ? Qt.AlignVCenter : Qt.AlignLeft
                                }

                                StateBadge {
                                    text: qsTr("Installed")
                                    statusType: "neutral"
                                    visible: modelRow.isInstalled && !modelRow.isSelected && !modelRow.isDownloading
                                    Layout.alignment: root.width >= 540 ? Qt.AlignVCenter : Qt.AlignLeft
                                }

                                StateBadge {
                                    text: qsTr("Downloading")
                                    statusType: "accent"
                                    visible: modelRow.isDownloading
                                    showDot: true
                                    pulsing: true
                                    Layout.alignment: root.width >= 540 ? Qt.AlignVCenter : Qt.AlignLeft
                                }

                                RowLayout {
                                    spacing: Theme.spacingXs
                                    Layout.alignment: root.width >= 540 ? Qt.AlignVCenter : Qt.AlignLeft

                                    StyledButton {
                                        text: qsTr("Cancel")
                                        size: "small"
                                        variant: "danger"
                                        visible: modelRow.isDownloading
                                        onClicked: {
                                            WhisperModelManager.cancelDownload(modelRow.modelId);
                                        }
                                    }

                                    StyledButton {
                                        text: qsTr("Download")
                                        size: "small"
                                        variant: "primary"
                                        visible: !modelRow.isInstalled && !modelRow.isDownloading
                                        enabled: !WhisperModelManager.isDownloadingAny
                                        onClicked: {
                                            WhisperModelManager.startDownload(modelRow.modelId);
                                        }
                                    }

                                    StyledButton {
                                        text: qsTr("Set Active")
                                        size: "small"
                                        variant: "secondary"
                                        visible: modelRow.isInstalled && !modelRow.isSelected && !modelRow.isDownloading
                                        onClicked: {
                                            WhisperModelManager.setSelectedModelId(modelRow.modelId);
                                        }
                                    }

                                    StyledButton {
                                        text: qsTr("Delete")
                                        size: "small"
                                        variant: "ghost"
                                        visible: modelRow.canDelete && !modelRow.isDownloading
                                        onClicked: {
                                            WhisperModelManager.deleteModel(modelRow.modelId);
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                visible: modelRow.isDownloading
                                Layout.topMargin: 2

                                RowLayout {
                                    Layout.fillWidth: true

                                    StyledText {
                                        text: qsTr("%1%").arg(Math.round(modelRow.progress * 100))
                                        variant: "caption"
                                        customWeight: Font.DemiBold
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                    }

                                    StyledText {
                                        text: WhisperModelManager.downloadBytesFormatted + (
                                                  modelRow.speedFormatted.length > 0 ? " (" + modelRow.speedFormatted
                                                                                       + ")" : "")
                                        variant: "caption"
                                        colorRole: "secondary"
                                    }
                                }

                                T.ProgressBar {
                                    Layout.fillWidth: true
                                    implicitHeight: 4
                                    from: 0.0
                                    to: 1.0
                                    value: Math.min(Math.max(modelRow.progress, 0.0), 1.0)

                                    background: Rectangle {
                                        implicitHeight: 4
                                        radius: Theme.radiusCircle
                                        color: Theme.controlBg
                                    }

                                    contentItem: Item {
                                        implicitHeight: 4
                                        Rectangle {
                                            anchors.left: parent.left
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            width: parent.width * Math.min(Math.max(modelRow.progress, 0.0), 1.0)
                                            radius: Theme.radiusCircle
                                            color: Theme.accentColor
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        StyledCard {
            title: qsTr("Hardware Acceleration")
            description: qsTr("GPU compute offloading status and inference backend diagnostics")

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMd

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    StyledText {
                        text: qsTr("Inference Engine Device")
                        variant: "body"
                        customWeight: Font.Medium
                    }

                    StyledText {
                        text: WhisperSttClient.isVulkanSupported ? qsTr(
                                                                       "Vulkan GPU acceleration is enabled in this build") :
                                                                   qsTr("Running via optimized CPU SIMD intrinsics (AVX2/AVX)")
                        variant: "caption"
                        colorRole: "secondary"
                    }
                }

                StateBadge {
                    text: WhisperSttClient.computeDevice
                    statusType: WhisperSttClient.isVulkanSupported ? "accent" : "neutral"
                }
            }
        }
    }
}
