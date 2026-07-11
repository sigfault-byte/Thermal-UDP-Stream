import QtQuick
import QtQuick.Layouts

Rectangle {
    id : root

    required property bool autoScale
    signal scaleModeRequested(bool autoEnabled)

    required property real minimumCelsius
    required property real maximumCelsius
    required property real meanCelsius
    required property int belowRangePixelCount
    required property int aboveRangePixelCount
    required property int inRangePixelCount

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
            Layout.fillWidth: true
            text: "Minimum: " + root.minimumCelsius.toFixed(2) + " °C"
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: "Maximum: " + root.maximumCelsius.toFixed(2) + " °C"
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: "Mean: " + root.meanCelsius.toFixed(2) + " °C"
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: "Pixels below 10 °C: " + root.belowRangePixelCount
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: "Pixels above 45 °C: " + root.aboveRangePixelCount
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: "In-range pixels: " + root.inRangePixelCount + " / 768"
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
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
            rolling: root.autoScale
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

        // Item {
        //     Layout.fillHeight: true
        // }

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
