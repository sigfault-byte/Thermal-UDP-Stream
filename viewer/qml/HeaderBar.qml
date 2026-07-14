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

    // Viewer-side selected refresh rate.
    // The packet timing fields remain the measured runtime truth.
    required property int refreshRateHz

    // Compact Qt/application log model exposed from C++.
    // The model is already filtered so frame-rate spam does not fill the header.
    required property var logModel

    signal commandButtonClicked()
    signal quantizationModeRequested(int mode)
    signal refreshRateRequested(int hz)

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
    property var refreshRateOptions: [
        { "hz": 1, "label": "1 Hz" },
        { "hz": 8, "label": "8 Hz" }
    ]

    color: "#1b1b22"
    radius: 8

    clip: true

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

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

        ColumnLayout {
            Layout.maximumWidth: 54
            Layout.minimumWidth: 0
            spacing: 4

            Repeater {
                model: root.refreshRateOptions

                Rectangle {
                    Layout.preferredWidth: 52
                    Layout.preferredHeight: 18
                    radius: 4

                    // The camera defaults to 1 Hz on boot.
                    // Highlight the refresh rate the viewer last had acknowledged.
                    color: modelData.hz === root.refreshRateHz
                        ? "#2f9e44"
                        : "#444451"
                    opacity: root.canSendCommand
                        ? 1.0
                        : 0.55

                    Text {
                        anchors.fill: parent
                        anchors.margins: 2
                        text: modelData.label
                        color: "white"
                        font.pixelSize: 11
                        font.bold:
                            modelData.hz === root.refreshRateHz
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled:
                            root.canSendCommand
                            && modelData.hz !== root.refreshRateHz
                        cursorShape: Qt.PointingHandCursor

                        onClicked: {
                            root.refreshRateRequested(
                                modelData.hz
                            )
                        }
                    }
                }
            }
        }

        ColumnLayout{
            Layout.preferredWidth: 190
            Layout.minimumWidth: 120

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: root.udpStatusText
                color: root.udpStateColor
                font.pixelSize: 16
                font.bold: root.isReceivingFrames || root.isStreamStale

            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: root.tcpStateText
                color: root.tcpStateColor
                font.pixelSize: 16
                font.bold: root.hasTcpEndpoint

            }
        }

        Rectangle {
            id: headerLogPanel

            Layout.preferredWidth: 440
            Layout.minimumWidth: 180
            Layout.preferredHeight: 40

            radius: 5
            color: "#101014"
            border.color: "#2b2b35"
            border.width: 1
            clip: true

            ListView {
                id: headerLogView

                anchors.fill: parent
                anchors.margins: 4

                // The list is read-only: it accepts wheel/drag scrolling but no input.
                model: root.logModel
                clip: true
                interactive: true
                boundsBehavior: Flickable.StopAtBounds
                spacing: 1

                delegate: Text {
                    width: headerLogView.width
                    height: 15
                    text:
                        timestampText + " " + message
                    color: entryColor
                    font.pixelSize: 10
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Connections {
                target: root.logModel

                function onCountChanged() {
                    // Keep the tiny display on the newest useful event.
                    // It remains scrollable with the mouse wheel for recent history.
                    headerLogView.positionViewAtEnd()
                }
            }
        }

        Item {
            // Flexible air gap: UDP/TCP and the log stay grouped on the left,
            // while the changing packet statistics stay pinned on the far right.
            Layout.fillWidth: true
        }

        Text {
            Layout.preferredWidth: 92
            Layout.minimumWidth: 92
            Layout.maximumWidth: 92
            text: "FPS: " + root.fpsText
            color: "#d8d8df"
            font.pixelSize: 17
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignRight
        }

        Text {
            Layout.preferredWidth: 118
            Layout.minimumWidth: 118
            Layout.maximumWidth: 118
            text: "recv Δ: " + root.receivedIntervalText
            color: "#d8d8df"
            font.pixelSize: 17
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignRight
        }

        Text {
            Layout.preferredWidth: 112
            Layout.minimumWidth: 112
            Layout.maximumWidth: 112
            text: "cam Δ: " + root.cameraIntervalText
            color: "#d8d8df"
            font.pixelSize: 17
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignRight
        }
    }
}
