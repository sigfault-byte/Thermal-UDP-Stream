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
