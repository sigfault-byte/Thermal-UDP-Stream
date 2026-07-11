import QtQuick
import QtQuick.Layouts

Rectangle {
    id : root

    required property bool rollingScale
    signal scaleModeRequested(bool autoEnabled)

    color: "#18181f"
    radius: 8

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        Text {
            text: "Frame statistics"
            color: "white"
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            text: "Minimum: --"
            color: "#d8d8df"
            font.pixelSize: 16
        }

        Text {
            text: "Maximum: --"
            color: "#d8d8df"
            font.pixelSize: 16
        }

        Text {
            text: "Mean: --"
            color: "#d8d8df"
            font.pixelSize: 16
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#3a3a44"
        }

        Text {
            text: "Display scale"
            color: "white"
            font.pixelSize: 18
            font.bold: true
        }

        RawRollingSwitch {
            rolling: root.rollingScale
            onModeRequested: rolling => {
                root.scaleModeRequested(rolling)
            }
        }

        Text {
            text: "Vmin: --"
            color: "#d8d8df"
            font.pixelSize: 16
        }

        Text {
            text: "Vmax: --"
            color: "#d8d8df"
            font.pixelSize: 16
        }

        Item {
            Layout.fillHeight: true
        }

        Text {
            text: "Packets lost: --"
            color: "#d8d8df"
            font.pixelSize: 16
        }

        Text {
            text: "Out of order: --"
            color: "#d8d8df"
            font.pixelSize: 16
        }

        Text {
            text: "FPS: --"
            color: "#d8d8df"
            font.pixelSize: 16
        }
    }
}
