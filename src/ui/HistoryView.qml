pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe
import "controls"

Item {
    id: root

    implicitWidth: 620
    implicitHeight: 540

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingMd

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingXs
            Layout.bottomMargin: Theme.spacingSm
            spacing: Theme.spacingMd

            ColumnLayout {
                spacing: 4

                StyledText {
                    text: qsTr("History")
                    variant: "heading"
                }

                StyledText {
                    text: qsTr("Recent speech transcriptions")
                    variant: "caption"
                    colorRole: "secondary"
                }
            }

            Item {
                Layout.fillWidth: true
            }

            StyledTextField {
                id: searchField
                placeholderText: qsTr("Search history…")
                Layout.fillWidth: true
                Layout.maximumWidth: 260
                Layout.minimumWidth: 140
                isSearchPill: true
                onTextChanged: TranscriptionModel.searchFilter = text
            }

            StyledButton {
                id: clearHistoryBtn
                text: qsTr("Clear History")
                iconSource: "qrc:/qt/qml/QtScribe/assets/icons/trash.svg"
                size: "small"
                enabled: TranscriptionModel.count > 0
                onClicked: {
                    TranscriptionModel.clearAll();
                }
            }
        }

        StyledCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0
            clip: true

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 32
                    color: Theme.cardBgElevated

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMd
                        anchors.rightMargin: Theme.spacingMd
                        spacing: Theme.spacingMd

                        StyledText {
                            text: qsTr("Time")
                            variant: "caption"
                            customWeight: Font.DemiBold
                            colorRole: "secondary"
                            verticalAlignment: Text.AlignVCenter
                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredWidth: root.width < 500 ? 95 : 130
                        }

                        StyledText {
                            text: qsTr("Transcription")
                            variant: "caption"
                            customWeight: Font.DemiBold
                            colorRole: "secondary"
                            verticalAlignment: Text.AlignVCenter
                            Layout.alignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                        }

                        StyledText {
                            text: qsTr("Copy")
                            variant: "caption"
                            customWeight: Font.DemiBold
                            colorRole: "secondary"
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredWidth: 72
                        }
                    }
                }

                StyledDivider {}

                ListView {
                    id: listView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: TranscriptionModel
                    clip: true
                    reuseItems: true
                    boundsBehavior: Flickable.StopAtBounds

                    add: Transition {
                        NumberAnimation {
                            property: "opacity"
                            from: 0
                            to: 1
                            duration: Theme.animNormal
                            easing.type: Easing.OutCubic
                        }
                        NumberAnimation {
                            property: "y"
                            from: -16
                            duration: Theme.animNormal
                            easing.type: Easing.OutCubic
                        }
                    }

                    remove: Transition {
                        NumberAnimation {
                            property: "opacity"
                            to: 0
                            duration: Theme.animFast
                            easing.type: Easing.InCubic
                        }
                    }

                    displaced: Transition {
                        NumberAnimation {
                            properties: "y"
                            duration: Theme.animNormal
                            easing.type: Easing.OutCubic
                        }
                    }

                    delegate: Rectangle {
                        id: delegateItem
                        required property int index
                        required property string formattedTimestamp
                        required property string text

                        property bool isCopied: false

                        ListView.onPooled: {
                            delegateItem.isCopied = false;
                            feedbackTimer.stop();
                        }

                        width: listView.width
                        implicitHeight: Math.max(rowContent.implicitHeight + (Theme.spacingSm * 2), 38)
                        color: delegateItem.index % 2 === 0 ? "transparent" : Theme.cardBgSubtle

                        RowLayout {
                            id: rowContent
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingMd
                            anchors.rightMargin: Theme.spacingMd
                            anchors.topMargin: Theme.spacingXs
                            anchors.bottomMargin: Theme.spacingXs
                            spacing: Theme.spacingMd

                            StyledText {
                                text: delegateItem.formattedTimestamp
                                variant: "caption"
                                colorRole: "secondary"
                                verticalAlignment: Text.AlignVCenter
                                Layout.preferredWidth: root.width < 500 ? 95 : 130
                                Layout.alignment: Qt.AlignVCenter
                            }

                            TextEdit {
                                text: delegateItem.text
                                font.pixelSize: Theme.fontSizeBody
                                color: Theme.textPrimary
                                wrapMode: TextEdit.WordWrap
                                readOnly: true
                                selectByMouse: true
                                verticalAlignment: TextEdit.AlignVCenter
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }

                            StyledButton {
                                id: copyBtn
                                text: delegateItem.isCopied ? qsTr("Copied!") : qsTr("Copy")
                                iconSource: delegateItem.isCopied ? "qrc:/qt/qml/QtScribe/assets/icons/check.svg" :
                                                                    "qrc:/qt/qml/QtScribe/assets/icons/copy.svg"
                                size: "small"
                                variant: delegateItem.isCopied ? "primary" : "secondary"
                                customAccentColor: delegateItem.isCopied ? Theme.colorSuccess : "transparent"
                                Layout.preferredWidth: 72
                                Layout.alignment: Qt.AlignVCenter

                                Timer {
                                    id: feedbackTimer
                                    interval: 1500
                                    onTriggered: delegateItem.isCopied = false
                                }

                                onClicked: {
                                    TranscriptionModel.copyToClipboard(delegateItem.text);
                                    delegateItem.isCopied = true;
                                    feedbackTimer.restart();
                                }
                            }
                        }

                        StyledDivider {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                        }
                    }

                    ColumnLayout {
                        anchors.centerIn: parent
                        visible: listView.count === 0
                        spacing: Theme.spacingSm

                        StyledText {
                            text: searchField.text.length > 0 ? qsTr("No matching transcriptions found.") : qsTr(
                                                                    "No Transcriptions Yet")
                            variant: "heading"
                            Layout.alignment: Qt.AlignHCenter
                        }

                        StyledText {
                            text: searchField.text.length > 0 ? qsTr("Try searching with different keywords.") : qsTr(
                                                                    "Press Ctrl+Shift+Space to start speaking.")
                            variant: "body"
                            colorRole: "secondary"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            StyledText {
                text: {
                    if (TranscriptionModel.count === 0) {
                        return qsTr("0 transcriptions");
                    }
                    if (searchField.text.length > 0) {
                        const filtered = TranscriptionModel.filteredCount;
                        const total = TranscriptionModel.totalCount;
                        return qsTr("Showing %1 of %2 transcriptions").arg(filtered).arg(total);
                    }
                    return qsTr("%1 transcriptions").arg(TranscriptionModel.count);
                }
                variant: "caption"
                colorRole: "secondary"
            }

            Item {
                Layout.fillWidth: true
            }

            StyledText {
                text: qsTr("Session history only — cleared when quitting")
                variant: "caption"
                colorRole: "secondary"
            }
        }
    }
}
