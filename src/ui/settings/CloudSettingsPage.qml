pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe
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
                text: qsTr("Cloud Dictation")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Configure cloud speech recognition providers, API credentials, and LLM text enhancement")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        EngineModeSwitcherCard {
            Layout.fillWidth: true
        }

        StyledCard {
            title: qsTr("Cloud Provider")
            description: qsTr("Choose the cloud backend provider for speech transcription and text enhancement")

            PreferenceRow {
                title: qsTr("Active Provider")
                description: CloudProviderModel.activeProviderDescription
                layoutVertical: false

                StyledComboBox {
                    id: providerCombo
                    Layout.preferredWidth: 220
                    textRole: "name"
                    valueRole: "providerId"
                    model: CloudProviderModel
                    currentIndex: CloudProviderModel.activeProviderIndex

                    onActivated: index => {
                        CloudProviderModel.activeProviderIndex = index;
                    }
                }
            }
        }

        Loader {
            id: providerSettingsLoader
            Layout.fillWidth: true
            source: CloudProviderModel.activeProviderComponent
        }
    }
}
