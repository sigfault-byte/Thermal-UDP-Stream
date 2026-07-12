import QtQuick
import QtQuick.Layouts

// Top status bar.

Rectangle {
    id: root

    // what is needed
    required property int udpPort
    required property bool hasReceivedFrame
    required property bool isReceivingFrames
    required property bool isStreamStale
    required property bool hasReceivedFrameInterval
    required property bool hasCameraFrameInterval
    required property real receivedFramesPerSecond
    required property int receivedFrameIntervalMs
    required property int cameraFrameIntervalMs

    property string title: "Padawan Thermal Viewer"
    property string udpEndpointText:
        "UDP(port " + root.udpPort + ")"
    property string udpStateText:
        root.isReceivingFrames
            ? "receiving"
            : root.isStreamStale
                ? "stale"
                : "listening"
    property color udpStateColor:
        root.isReceivingFrames
            ? "#8fd19e"
            : root.isStreamStale
                ? "#f08080"
                : "#777783"
    property string fpsText:
        root.isReceivingFrames && root.hasReceivedFrameInterval
            ? root.receivedFramesPerSecond.toFixed(1)
            : "---"
    property string receivedIntervalText:
        root.isReceivingFrames && root.hasReceivedFrameInterval
            ? root.receivedFrameIntervalMs + " ms"
            : "---"
    property string cameraIntervalText:
        root.isReceivingFrames && root.hasCameraFrameInterval
            ? root.cameraFrameIntervalMs + " ms"
            : "---"

    color: "#1b1b22"
    radius: 8

    clip: true

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 24

        Text {
            Layout.maximumWidth: root.width * 0.4
            Layout.minimumWidth: 0
            text: root.title
            color: "white"
            font.pixelSize: 22
            font.bold: true
            elide: Text.ElideRight
        }

        Item {
            Layout.fillWidth: true
        }

        Text {
            Layout.maximumWidth: 170
            Layout.minimumWidth: 0
            text: root.udpEndpointText
            color: "#d8d8df"
            font.pixelSize: 17
            elide: Text.ElideRight
        }

        Text {
            Layout.maximumWidth: 100
            Layout.minimumWidth: 0
            text: root.udpStateText
            color: root.udpStateColor
            font.pixelSize: 17
            font.bold: root.isReceivingFrames || root.isStreamStale
            elide: Text.ElideRight
        }

        Text {
            Layout.maximumWidth: 110
            Layout.minimumWidth: 0
            text: "FPS: " + root.fpsText
            color: "#d8d8df"
            font.pixelSize: 17
            elide: Text.ElideRight
        }

        Text {
            Layout.maximumWidth: 130
            Layout.minimumWidth: 0
            text: "recv Δ: " + root.receivedIntervalText
            color: "#d8d8df"
            font.pixelSize: 17
            elide: Text.ElideRight
        }

        Text {
            Layout.maximumWidth: 130
            Layout.minimumWidth: 0
            text: "cam Δ: " + root.cameraIntervalText
            color: "#d8d8df"
            font.pixelSize: 17
            elide: Text.ElideRight
        }
    }
}
