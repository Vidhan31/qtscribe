pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
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

            Flickable {
                id: licenseFlickable
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: width
                contentHeight: licenseTextEdit.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                TextEdit {
                    id: licenseTextEdit
                    width: licenseFlickable.width
                    readOnly: true
                    selectByMouse: true
                    text: LicenseHelper.licenseText
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                    font.family: "Monospace"
                    font.pixelSize: Theme.fontSizeCaption
                    color: Theme.textPrimary
                    selectionColor: Theme.accentColor
                    selectedTextColor: Theme.textOnAccent
                    padding: Theme.spacingMd
                }
            }
        }
    }
}
