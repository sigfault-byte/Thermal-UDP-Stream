import QtQuick
import QtQuick.Layouts

Rectangle {
    id : root

    required property bool autoScale
    signal scaleModeRequested(bool autoEnabled)

    required property int imageRevision
    required property real minimumCelsius
    required property real maximumCelsius
    required property real meanCelsius
    required property int belowRangePixelCount
    required property int aboveRangePixelCount
    required property int inRangePixelCount
    required property var receivedDatagramCount
    required property var completedFrameCount
    required property var rejectedDatagramCount
    required property var latestReceivedFrameId
    required property bool hotspotValid
    required property bool hotspotPeakAboveRange
    required property real hotspotPeakTemperatureCelsius
    required property real hotspotMeanTemperatureCelsius
    required property real hotspotCenterX
    required property real hotspotCenterY
    required property real hotspotRadiusPixels
    required property int hotspotHotPixelCount
    required property int hotspotTotalPixelCount
    required property real hotspotScore

    color: "#18181f"
    radius: 8

    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: "Statistics"
            color: "white"
            font.pixelSize: 18
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            columns: 2
            columnSpacing: 20
            rowSpacing: 8

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Frame"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Minimum: " + root.minimumCelsius.toFixed(2) + " °C"
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Maximum: " + root.maximumCelsius.toFixed(2) + " °C"
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Mean: " + root.meanCelsius.toFixed(2) + " °C"
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "In-range pixels: " + root.inRangePixelCount + " / 768"
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Below 10 °C: " + root.belowRangePixelCount
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Above 45 °C: " + root.aboveRangePixelCount
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Receiver"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Datagrams received (Session): " + root.receivedDatagramCount
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Completed frames (Session): " + root.completedFrameCount
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Rejected datagrams: " + root.rejectedDatagramCount
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Latest frame ID: " + root.latestReceivedFrameId
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }
            }

        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#3a3a44"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.alignment: Qt.AlignTop
            spacing: 20

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.alignment: Qt.AlignTop
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Display scale"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                }

                RawRollingSwitch {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    rolling: root.autoScale
                    onModeRequested: rolling => {
                        root.scaleModeRequested(rolling)
                    }
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Vmin: "
                          + (root.autoScale
                              ? root.minimumCelsius.toFixed(2)
                              : "10.00")
                          + " °C"
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: "Vmax: "
                          + (root.autoScale
                              ? root.maximumCelsius.toFixed(2)
                              : "45.00")
                          + " °C"
                    color: "#d8d8df"
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }
            }

            HotSpotStats {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.alignment: Qt.AlignTop

                imageRevision: root.imageRevision
                hotspotValid: root.hotspotValid
                hotspotPeakAboveRange: root.hotspotPeakAboveRange
                hotspotPeakTemperatureCelsius:
                    root.hotspotPeakTemperatureCelsius
                hotspotMeanTemperatureCelsius:
                    root.hotspotMeanTemperatureCelsius
                hotspotCenterX: root.hotspotCenterX
                hotspotCenterY: root.hotspotCenterY
                hotspotRadiusPixels: root.hotspotRadiusPixels
                hotspotHotPixelCount: root.hotspotHotPixelCount
                hotspotTotalPixelCount: root.hotspotTotalPixelCount
                hotspotScore: root.hotspotScore
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
        }
    }
}
