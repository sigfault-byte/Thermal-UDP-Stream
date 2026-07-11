import QtQuick
import QtQuick.Layouts

// Top status bar.

Rectangle {
    id: root

    // what is needed
    required property int frameId
    property string title: "Padawan Thermal Viewer"
    property string mockDataUDP : "UDP: connected"

    color: "#1b1b22"
    radius: 8

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 24

        Text {
            text: root.title
            color: "white"
            font.pixelSize: 22
            font.bold: true
        }

        Item {
            Layout.fillWidth: true
        }

        Text {
            text: "Frame: " + root.frameId
            color: "#d8d8df"
            font.pixelSize: 17
        }

        Text {
            text: root.mockDataUDP
            color: "#d8d8df"
            font.pixelSize: 17
        }
    }
}
