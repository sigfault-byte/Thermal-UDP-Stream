import QtQuick
import QtQuick.Window


import QtQuick
import QtQuick.Window

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls

Window {
    width: 1200
    height: 800
    visible: true
    title: "Padawan Thermal Viewer"
    color: "#101014"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12


        // Top status bar.
        HeaderBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 64

            udpPort: receiverUdpPort
            hasReceivedFrame: frameModel.hasReceivedFrame
            isReceivingFrames: frameModel.isReceivingFrames
            isStreamStale: frameModel.isStreamStale
            hasReceivedFrameInterval:
                frameModel.hasReceivedFrameInterval
            hasCameraFrameInterval:
                frameModel.hasCameraFrameInterval
            receivedFramesPerSecond:
                frameModel.receivedFramesPerSecond
            receivedFrameIntervalMs:
                frameModel.receivedFrameIntervalMs
            cameraFrameIntervalMs:
                frameModel.cameraFrameIntervalMs
        }

        // Main content area.
        RowLayout {
            id: dashboardRow

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            spacing: 12

            // Thermal image panel.
            ThermalPanel {
                Layout.fillHeight: true
                Layout.minimumWidth: 0
                Layout.preferredWidth:
                    Math.max(
                        300,
                        Math.min(
                            520,
                            (dashboardRow.height - 88) * 4 / 3 + 32
                        )
                    )
                imageSource:
                    "image://thermal/frame?revision="
                    // also send image revision property
                    + frameModel.imageRevision
                    hotspotValid: frameModel.hotspotValid

                    hotspotX: frameModel.hotspotPeakX
                    hotspotY: frameModel.hotspotPeakY
                    hotspotAboveRange: frameModel.hotspotPeakAboveRange
                    hotspotTemperatureCelsius:
                        frameModel.hotspotPeakTemperatureCelsius
                    hotspotCenterX: frameModel.hotspotCenterX
                    hotspotCenterY: frameModel.hotspotCenterY
                    hotspotRadiusPixels: frameModel.hotspotRadiusPixels
            }

            // Statistics and controls panel.
            StatisticsPanel {
                Layout.fillWidth: true
                Layout.preferredWidth: 620
                Layout.minimumWidth: 0
                Layout.fillHeight: true

                minimumCelsius: frameModel.minimumCelsius
                maximumCelsius: frameModel.maximumCelsius
                meanCelsius: frameModel.meanCelsius

                inRangePixelCount: frameModel.inRangePixelCount
                belowRangePixelCount: frameModel.belowRangePixelCount
                aboveRangePixelCount: frameModel.aboveRangePixelCount
                receivedDatagramCount: frameModel.receivedDatagramCount
                completedFrameCount: frameModel.completedFrameCount
                rejectedDatagramCount: frameModel.rejectedDatagramCount
                latestReceivedFrameId: frameModel.latestReceivedFrameId

                autoScale:
                    frameModel.scaleMode === FrameModel.Auto

                onScaleModeRequested: autoEnabled => {
                    frameModel.scaleMode =
                        autoEnabled
                            ? FrameModel.Auto
                            : FrameModel.Raw
                }
            }
        }

        //last row
        // Future chart / packet timing area.
        TemperatureGraph{
        Layout.fillWidth: true
        Layout.preferredHeight: 220
        }
        // TimingPanel {
        //     Layout.fillWidth: true
        //     Layout.preferredHeight: 220
        // }
    }
}
