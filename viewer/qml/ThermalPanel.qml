import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    // Inputs supplied by the parent component.
    required property url imageSource
    property string title: "Live thermal frame"

    color: "#18181f"
    radius: 8

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Text {
            text: root.title
            color: "white"
            font.pixelSize: 18
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            color: "#09090c"
            radius: 6

            Image {
                anchors.fill: parent
                anchors.margins: 20

                source: root.imageSource
                fillMode: Image.PreserveAspectFit
                smooth: false
                cache: false
            }
        }
    }
}
