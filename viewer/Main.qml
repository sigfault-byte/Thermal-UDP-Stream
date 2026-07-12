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

                    hotspotX: frameModel.hotspotX
                    hotspotY: frameModel.hotspotY
                    hotspotAboveRange: frameModel.hotspotAboveRange
                    hotspotTemperatureCelsius:
                        frameModel.hotspotTemperatureCelsius
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
        TimingPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 220
        }
    }
}

// Window {
//     width: 900
//     height: 700
//     visible: true
//     title: "Padawan Thermal Viewer"

//     Rectangle {
//         anchors.fill: parent
//         color: "#101014"

//         Image {
//             id: thermalImage

//             anchors.fill: parent
//             anchors.margins: 40

//             /*
//              * image://thermal
//              *     selects the C++ image provider.
//              *
//              * /frame
//              *     is the requested image ID.
//              *
//              * The revision changes after every received frame,
//              * forcing source to become a new URL.
//              */
//             source:
//                 "image://thermal/frame?revision="
//                 + frameModel.imageRevision

//             fillMode: Image.PreserveAspectFit

//             // Keep each 32 × 24 sensor cell crisp when enlarged.
//             smooth: false

//             // Do not retain an older provider result in QML's cache.
//             cache: false
//         }

//         Rectangle {
//             anchors.left: parent.left
//             anchors.top: parent.top
//             anchors.margins: 16

//             width: frameText.implicitWidth + 24
//             height: frameText.implicitHeight + 16

//             color: "#99000000"
//             radius: 6

//             Text {
//                 id: frameText

//                 anchors.centerIn: parent

//                 text: "Frame: " + frameModel.frameId
//                 color: "white"
//                 font.pixelSize: 20
//             }
//         }
//     }
// }

// Window {
//     width: 800
//     height: 600
//     visible: true
//     title: "Padawan Thermal Viewer"

//     Rectangle {
//         anchors.fill: parent

//         Text {
//             anchors.centerIn: parent

//             // This is a binding.
//             // When frameModel.frameId changes, Qt reevaluates this expression.
//             text: "Frame: " + frameModel.frameId

//             font.pixelSize: 32
//         }
//     }
// }
