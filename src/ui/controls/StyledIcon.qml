pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import QtScribe

Item {
    id: root

    property url source: ""
    property color color: Theme.textSecondary
    property int size: 16

    implicitWidth: root.size
    implicitHeight: root.size

    Image {
        id: iconImg
        anchors.fill: parent
        source: root.source
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        visible: false
        smooth: true
        mipmap: true
    }

    MultiEffect {
        anchors.fill: iconImg
        source: iconImg
        colorization: 1.0
        colorizationColor: root.color
        visible: iconImg.status === Image.Ready
    }
}
