import QtQuick
import QtQuick.Window

Window {
    width: 800
    height: 600
    visible: true
    title: "Padawan Thermal Viewer"

    Rectangle {
        anchors.fill: parent

        Text {
            anchors.centerIn: parent

            // This is a binding.
            // When frameModel.frameId changes, Qt reevaluates this expression.
            text: "Frame: " + frameModel.frameId

            font.pixelSize: 32
        }
    }
}
