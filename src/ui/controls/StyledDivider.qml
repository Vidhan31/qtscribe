pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe

Rectangle {
    id: root

    property int orientation: Qt.Horizontal
    property color dividerColor: Theme.cardBorder
    property int thickness: 1

    Layout.fillWidth: orientation === Qt.Horizontal
    Layout.fillHeight: orientation === Qt.Vertical

    implicitWidth: orientation === Qt.Horizontal ? 1 : root.thickness
    implicitHeight: orientation === Qt.Horizontal ? root.thickness : 1

    color: root.dividerColor
}
