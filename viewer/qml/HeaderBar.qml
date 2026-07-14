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

    signal commandButtonClicked()

    property string title: "Padawan Thermal Viewer"
    property string udpEndpointText:
        "UDP(port " + root.udpPort + ")"
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

        ColumnLayout {
            Layout.maximumWidth: 210
            Layout.minimumWidth: 0
            spacing: 2

            // Existing thermal frame UDP status.
            // This port continues to receive image frames independently.
            Text {
                Layout.fillWidth: true
                text: root.udpEndpointText
                color: "#d8d8df"
                font.pixelSize: 15
                elide: Text.ElideRight
            }

            // Newly discovered TCP command endpoint status.
            // This only means an endpoint is prepared; no TCP socket is open yet.
            Text {
                Layout.fillWidth: true
                text: root.tcpStateText
                color: root.tcpStateColor
                font.pixelSize: 15
                font.bold: root.hasTcpEndpoint
                elide: Text.ElideRight
            }
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

        // Single camera command button.
        // This is a plain Rectangle instead of a Qt Quick Controls Button,
        // because the native macOS style does not allow background/contentItem
        // customization without switching the whole app to a non-native style.
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
