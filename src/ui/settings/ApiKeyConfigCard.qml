pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import "../controls"

StyledCard {
    id: root

    property string rowTitle: qsTr("API Key")
    property string rowDescription: ""
    property string apiKey: ""
    property string placeholderText: ""
    property string helpText: ""
    property string linkButtonText: qsTr("Get API Key")
    property string linkUrl: ""

    signal saveRequested(string key)

    PreferenceRow {
        title: root.rowTitle
        description: root.rowDescription
        layoutVertical: true

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            StyledTextField {
                id: apiKeyInput
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                echoMode: showKeyCheck.checked ? T.TextField.Normal : T.TextField.Password
                text: root.apiKey
                placeholderText: root.placeholderText
            }

            T.CheckBox {
                id: showKeyCheck
                text: qsTr("Show")
                font.pixelSize: Theme.fontSizeCaption
                Layout.alignment: Qt.AlignVCenter
                implicitHeight: 34
                implicitWidth: indicator.implicitWidth + Theme.spacingXs + showLabel.implicitWidth + leftPadding
                               + rightPadding
                leftPadding: 4
                rightPadding: 4

                indicator: Rectangle {
                    implicitWidth: 16
                    implicitHeight: 16
                    x: showKeyCheck.leftPadding
                    y: Math.round((showKeyCheck.height - height) / 2)
                    radius: Theme.radiusXs
                    color: showKeyCheck.checked ? Theme.accentColor : Theme.controlBg
                    border.color: showKeyCheck.checked ? Theme.accentColor : Theme.controlBorder
                    border.width: 1

                    StyledIcon {
                        anchors.centerIn: parent
                        source: "qrc:/qt/qml/QtScribe/assets/icons/check.svg"
                        size: 10
                        color: Theme.textOnAccent
                        visible: showKeyCheck.checked
                    }
                }

                contentItem: StyledText {
                    id: showLabel
                    text: showKeyCheck.text
                    variant: "caption"
                    colorRole: "secondary"
                    leftPadding: showKeyCheck.indicator.width + Theme.spacingXs
                    verticalAlignment: Text.AlignVCenter
                }
            }

            StyledButton {
                id: saveKeyBtn
                text: qsTr("Save")
                iconSource: "qrc:/qt/qml/QtScribe/assets/icons/save.svg"
                variant: "primary"
                size: "medium"
                Layout.alignment: Qt.AlignVCenter
                onClicked: {
                    root.saveRequested(apiKeyInput.text);
                }
            }
        }
    }

    StyledDivider {
        visible: root.helpText.length > 0 || root.linkUrl.length > 0
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacingSm
        visible: root.helpText.length > 0 || root.linkUrl.length > 0

        StyledText {
            text: root.helpText
            variant: "caption"
            colorRole: "secondary"
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            visible: root.helpText.length > 0
        }

        StyledButton {
            id: getKeyBtn
            text: root.linkButtonText
            variant: "flat"
            size: "small"
            visible: root.linkUrl.length > 0
            onClicked: {
                if (root.linkUrl.length > 0) {
                    Qt.openUrlExternally(root.linkUrl);
                }
            }
        }
    }
}
