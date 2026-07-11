import QtQuick
import QtQuick.Layouts


Rectangle {
    id: root

    color: "#18181f"
    radius: 8

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        Text {
            text: "Timing and packet history"
            color: "white"
            font.pixelSize: 18
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            color: "#0d0d12"
            radius: 6

            Text {
                anchors.centerIn: parent
                text: "Time-series view will live here"
                color: "#777783"
                font.pixelSize: 16
            }
        }
    }
}
