import QtQuick
import QtQuick.Window


import QtQuick
import QtQuick.Window

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls

Window {
    id: mainWindow

    width: 1200
    height: 800
    visible: true
    title: "Padawan Thermal Viewer"
    color: "#101014"

    // Copy the C++ context property into a window property.
    // This avoids an ambiguous HeaderBar binding with the same property name.
    property int padawanCommandTcpPort: commandTcpPort

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12


        // Top status bar.
        HeaderBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 64

            udpPort: receiverUdpPort

            // Show the fixed command port until discovery fills in the IP.
            commandTcpPort: mainWindow.padawanCommandTcpPort

            hasReceivedFrame: frameModel.hasReceivedFrame
            isReceivingFrames: frameModel.isReceivingFrames
            isStreamStale: frameModel.isStreamStale

            // Show whether UDP discovery has prepared a TCP endpoint.
            hasTcpEndpoint: tcpCommandClient.hasEndpoint

            // Show the backend-formatted discovered "ip:port" endpoint.
            tcpEndpointText: tcpCommandClient.endpointText

            // Enable the command button only when the backend can send.
            canSendCommand: tcpCommandClient.canSendCommand

            // Let the backend provide START, STOP, or WAIT.
            commandButtonText: tcpCommandClient.commandButtonText

            // Let the backend provide active or pending button color.
            commandButtonColor: tcpCommandClient.commandButtonColor

            // Display the quantization mode from the latest UDP frame.
            quantizationMode: frameModel.quantizationMode

            // Display the human range for the latest UDP frame mode.
            quantizationRangeText: frameModel.quantizationRangeText

            onCommandButtonClicked: {
                // Toggle based on the last acknowledged ESP32 running state.
                if (tcpCommandClient.cameraRunning) {
                    tcpCommandClient.sendStopCommand()
                } else {
                    tcpCommandClient.sendStartCommand()
                }
            }

            onQuantizationModeRequested: mode => {
                tcpCommandClient.sendSetQuantizationCommand(mode)
            }

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

                imageRevision: frameModel.imageRevision
                minimumCelsius: frameModel.minimumCelsius
                maximumCelsius: frameModel.maximumCelsius
                meanCelsius: frameModel.meanCelsius
                quantizationMinimumCelsius:
                    frameModel.quantizationMinimumCelsius
                quantizationMaximumCelsius:
                    frameModel.quantizationMaximumCelsius

                inRangePixelCount: frameModel.inRangePixelCount
                belowRangePixelCount: frameModel.belowRangePixelCount
                aboveRangePixelCount: frameModel.aboveRangePixelCount
                receivedDatagramCount: frameModel.receivedDatagramCount
                completedFrameCount: frameModel.completedFrameCount
                rejectedDatagramCount: frameModel.rejectedDatagramCount
                latestReceivedFrameId: frameModel.latestReceivedFrameId
                hotspotValid: frameModel.hotspotValid
                hotspotPeakTemperatureCelsius:
                    frameModel.hotspotPeakTemperatureCelsius
                hotspotMeanTemperatureCelsius:
                    frameModel.hotspotMeanTemperatureCelsius
                hotspotPeakAboveRange:
                    frameModel.hotspotPeakAboveRange
                hotspotCenterX: frameModel.hotspotCenterX
                hotspotCenterY: frameModel.hotspotCenterY
                hotspotRadiusPixels: frameModel.hotspotRadiusPixels
                hotspotHotPixelCount: frameModel.hotspotHotPixelCount
                hotspotTotalPixelCount: frameModel.hotspotTotalPixelCount
                hotspotScore: frameModel.hotspotScore

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
