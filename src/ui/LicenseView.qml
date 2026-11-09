pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import "controls"

Item {
    id: root

    implicitWidth: 620
    implicitHeight: 540

    ColumnLayout {
        anchors.fill: root
        spacing: Theme.spacingMd

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingXs
            Layout.bottomMargin: Theme.spacingSm
            spacing: 4

            StyledText {
                text: qsTr("License")
                variant: "heading"
            }

            StyledText {
                text: qsTr("GNU General Public License v3.0")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        StyledCard {
            id: licenseContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0
            clip: true

            T.ScrollView {
                id: licenseScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                T.TextArea {
                    id: licenseTextArea
                    readOnly: true
                    selectByMouse: true
                    text: LicenseHelper.licenseText
                    textFormat: Text.PlainText
                    wrapMode: TextArea.Wrap
                    font.family: "Monospace"
                    font.pixelSize: Theme.fontSizeCaption
                    color: Theme.textPrimary
                    selectionColor: Theme.accentColor
                    selectedTextColor: Theme.textOnAccent
                    background: null
                    padding: Theme.spacingMd
                }
            }
        }
    }
}
