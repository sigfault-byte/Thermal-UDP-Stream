import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: root

    // Inputs supplied by the parent component.
    required property url imageSource

    required property bool hotspotValid
    required property int hotspotX
    required property int hotspotY
    required property bool hotspotAboveRange
    required property real hotspotTemperatureCelsius

    required property real hotspotCenterX
    required property real hotspotCenterY
    required property real hotspotRadiusPixels

    property string title: "Live thermal frame"

    property real pixelWidth:
        thermalImage.paintedWidth / 32

    property real pixelHeight:
        thermalImage.paintedHeight / 24

    property real imageOffsetX:
        (thermalImage.width - thermalImage.paintedWidth) / 2

    property real imageOffsetY:
        (thermalImage.height - thermalImage.paintedHeight) / 2

    property int displayedHotspotX:
        31 - hotspotX

    color: "#18181f"
    radius: 8

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: root.title
                color: "white"
                font.pixelSize: 18
                font.bold: true

                Layout.fillWidth: true
            }

            Switch {
                id: hotspotOverlaySwitch

                text: "Hotspot"
                checked: false
            }
        }
        Rectangle {
            Layout.fillWidth: true
            visible: hotspotOverlaySwitch.checked

            implicitHeight: settingsLayout.implicitHeight + 20

            color: "#202028"
            radius: 6

            RowLayout {
                id: settingsLayout

                anchors.fill: parent
                anchors.margins: 10
                spacing: 16

                // Controls here.
                RowLayout {
                    Text {
                        text:
                            "Tolerance: "
                            + frameModel.hotspotTemperatureToleranceCelsius.toFixed(1)
                            + " °C"

                        color: "white"
                    }

                    Slider {
                        from: 0.1
                        to: 5.0
                        stepSize: 0.1

                        value:
                            frameModel.hotspotTemperatureToleranceCelsius

                        onMoved:
                            frameModel.hotspotTemperatureToleranceCelsius =
                                value
                    }
                    ColumnLayout {
                        Text {
                            text:
                                "Cold penalty: "
                                + frameModel.hotspotColdPixelPenalty.toFixed(1)

                            color: "white"
                        }

                        Slider {
                            from: 0.0
                            to: 5.0
                            stepSize: 0.1

                            value:
                                frameModel.hotspotColdPixelPenalty

                            onMoved:
                                frameModel.hotspotColdPixelPenalty =
                                    value
                        }
                    }
                    ColumnLayout {
                        Text {
                            text:
                                "Max radius: "
                                + frameModel.hotspotMaximumRadiusPixels
                                + " px"

                            color: "white"
                        }

                        Slider {
                            from: 1
                            to: 20
                            stepSize: 1

                            value:
                                frameModel.hotspotMaximumRadiusPixels

                            onMoved:
                                frameModel.hotspotMaximumRadiusPixels =
                                    Math.round(value)
                        }
                    }
                }
            }
        }

        Rectangle {
            id: imageContainer

            Layout.fillWidth: true
            Layout.fillHeight: true

            color: "#09090c"
            radius: 6

            Image {
                id: thermalImage

                anchors.fill: parent
                anchors.margins: 20

                source: root.imageSource
                fillMode: Image.PreserveAspectFit
                smooth: false
                cache: false
            }

            Rectangle {
                id: hotspotMarker

                property real pixelWidth:
                    thermalImage.paintedWidth / 32

                property real pixelHeight:
                    thermalImage.paintedHeight / 24

                property real imageOffsetX:
                    (thermalImage.width - thermalImage.paintedWidth) / 2

                property real imageOffsetY:
                    (thermalImage.height - thermalImage.paintedHeight) / 2

                property real displayedCenterX:
                    31 - frameModel.hotspotCenterX

                property real radiusX:
                    frameModel.hotspotRadiusPixels * pixelWidth

                property real radiusY:
                    frameModel.hotspotRadiusPixels * pixelHeight

                x: thermalImage.x
                   + imageOffsetX
                   + displayedCenterX * pixelWidth
                   - radiusX

                y: thermalImage.y
                   + imageOffsetY
                   + frameModel.hotspotCenterY * pixelHeight
                   - radiusY

                width: radiusX * 2
                height: radiusY * 2

                radius: width / 2

                visible:
                    hotspotOverlaySwitch.checked
                    && frameModel.hotspotValid

                color: "transparent"
                border.color: "red"
                border.width: 2
                z: 10
            }

            // Rectangle {
            //     id: hotspotMarker
            //     visible:
            //         hotspotOverlaySwitch.checked
            //         && hotspotValid

            //     x: thermalImage.x
            //        + imageOffsetX
            //        + displayedHotspotX * pixelWidth

            //     y: thermalImage.y
            //        + imageOffsetY
            //        + hotspotY * pixelHeight

            //     width: pixelWidth
            //     height: pixelHeight



            //     color: "transparent"
            //     border.color: "red"
            //     border.width: 2

            //     z: 10
            // }
        }
    }
}
