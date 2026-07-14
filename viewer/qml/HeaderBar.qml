import QtQuick
import QtQuick.Layouts

// Top status bar.

Rectangle {
    id: root

    // what is needed
    required property int udpPort

    // Fixed TCP command port shown before a camera IP has been discovered.
    required property int commandTcpPort

    required property bool hasReceivedFrame
    required property bool isReceivingFrames
    required property bool isStreamStale

    // True after the backend has prepared a discovered TCP endpoint.
    required property bool hasTcpEndpoint

    required property bool hasReceivedFrameInterval
    required property bool hasCameraFrameInterval
    required property real receivedFramesPerSecond
    required property int receivedFrameIntervalMs
    required property int cameraFrameIntervalMs

    // Backend-formatted "ip:port" text for the discovered command endpoint.
    required property string tcpEndpointText

    // True when the backend allows a START or STOP command click.
    required property bool canSendCommand

    // Button text supplied by the backend: START, STOP, or WAIT.
    required property string commandButtonText

    // Button color supplied by the backend.
    required property string commandButtonColor

    // Latest quantization mode reported by the UDP thermal frame.
    required property int quantizationMode

    // Human-readable range for the latest UDP thermal frame mode.
    required property string quantizationRangeText

    signal commandButtonClicked()
    signal quantizationModeRequested(int mode)

    property string udpEndpointText:
        "UDP:" + root.udpPort
    property string udpStatusText:
        root.udpEndpointText + " " + root.udpStateText
    property string tcpStateText:
        root.hasTcpEndpoint
            ? "TCP: " + root.tcpEndpointText
            : "TCP: --:" + root.commandTcpPort
    property string udpStateText:
        root.isReceivingFrames
            ? "receiving"
            : root.isStreamStale
                ? "stale"
                : "listening"
    property color tcpStateColor:
        root.hasTcpEndpoint
            ? "#8fd19e"
            : "#777783"
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
    property var quantizationOptions: [
        { "mode": 1, "label": "10-45" },
        { "mode": 2, "label": "20-60" },
        { "mode": 3, "label": "0-100" }
    ]

    color: "#1b1b22"
    radius: 8

    clip: true

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // Primary stream command comes first in the instrument strip.
        Rectangle {
            id: commandButton

            Layout.preferredWidth: 86
            Layout.preferredHeight: 34

            radius: 6
            color: root.commandButtonColor
            opacity: root.canSendCommand
                ? 1.0
                : 0.65

            Text {
                anchors.fill: parent
                anchors.margins: 6
                text: root.commandButtonText
                color: "white"
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.canSendCommand
                cursorShape: Qt.PointingHandCursor

                onClicked: {
                    root.commandButtonClicked()
                }
            }
        }

        ColumnLayout {
            Layout.maximumWidth: 220
            Layout.minimumWidth: 0
            spacing: 2

            // Display the mode reported by the newest UDP frame.
            // This is the real current quantization mode for visible pixels.
            Text {
                Layout.fillWidth: true
                text: "Q: " + root.quantizationRangeText
                color: "#d8d8df"
                font.pixelSize: 14
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Repeater {
                    model: root.quantizationOptions

                    Rectangle {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 22
                        radius: 4

                        // Highlight the mode from the last received frame.
                        // Disable clicks while any TCP command is pending.
                        color: modelData.mode === root.quantizationMode
                            ? "#2f9e44"
                            : "#444451"
                        opacity: root.canSendCommand
                            ? 1.0
                            : 0.55

                        Text {
                            anchors.fill: parent
                            anchors.margins: 3
                            text: modelData.label
                            color: "white"
                            font.pixelSize: 11
                            font.bold:
                                modelData.mode === root.quantizationMode
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled:
                                root.canSendCommand
                                && modelData.mode !== root.quantizationMode
                            cursorShape: Qt.PointingHandCursor

                            onClicked: {
                                root.quantizationModeRequested(
                                    modelData.mode
                                )
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout{
            Layout.fillWidth: true

            Text {
                Layout.maximumWidth: 130
                Layout.minimumWidth: 0
                text: root.udpStatusText
                color: root.udpStateColor
                font.pixelSize: 16
                font.bold: root.isReceivingFrames || root.isStreamStale

            }

            Text {
                Layout.maximumWidth: 220
                Layout.minimumWidth: 0
                text: root.tcpStateText
                color: root.tcpStateColor
                font.pixelSize: 16
                font.bold: root.hasTcpEndpoint

            }
        }

        Item {
            Layout.fillWidth: true
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
