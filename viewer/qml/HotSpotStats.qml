import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property int imageRevision
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

    property bool hasPreviousHotspot: false
    property real previousHotspotCenterX: 0.0
    property real previousHotspotCenterY: 0.0
    property real previousHotspotRadiusPixels: 0.0
    property real lastHotspotCenterX: 0.0
    property real lastHotspotCenterY: 0.0
    property real lastHotspotRadiusPixels: 0.0
    property int lastTrackedImageRevision: -1
    property int validHotspotSampleCount: 0

    readonly property real hotspotDeltaX:
        hotspotValid && hasPreviousHotspot
            ? hotspotCenterX - previousHotspotCenterX
            : 0.0

    readonly property real hotspotDeltaY:
        hotspotValid && hasPreviousHotspot
            ? hotspotCenterY - previousHotspotCenterY
            : 0.0

    readonly property real hotspotMoveDistance:
        Math.sqrt(
            hotspotDeltaX * hotspotDeltaX
            + hotspotDeltaY * hotspotDeltaY
        )

    readonly property real hotspotRadiusDelta:
        hotspotValid && hasPreviousHotspot
            ? hotspotRadiusPixels - previousHotspotRadiusPixels
            : 0.0

    readonly property string hotspotMovementText:
        !hotspotValid
            ? "No hotspot"
            : validHotspotSampleCount < 2
                ? "First sample"
                : hotspotMoveDistance <= 1.0
                    ? "Stable"
                    : "Moved"

    readonly property string hotspotSizeTrendText:
        !hotspotValid
            ? "No hotspot"
            : validHotspotSampleCount < 2
                ? "First sample"
                : Math.abs(hotspotRadiusDelta) < 0.25
                    ? "Stable"
                    : hotspotRadiusDelta > 0
                        ? "Growing"
                        : "Shrinking"

    spacing: 8

    function hotspotTemperatureText(value) {
        if (!hotspotValid || value <= 0.0)
            return "n/a"

        return value.toFixed(2) + " °C"
    }

    function hotspotPointText(x, y) {
        return "(" + x.toFixed(1) + ", " + y.toFixed(1) + ")"
    }

    function updateHotspotHistory() {
        if (lastTrackedImageRevision === imageRevision)
            return

        lastTrackedImageRevision = imageRevision

        if (!hotspotValid)
            return

        if (hasPreviousHotspot) {
            previousHotspotCenterX = lastHotspotCenterX
            previousHotspotCenterY = lastHotspotCenterY
            previousHotspotRadiusPixels = lastHotspotRadiusPixels
        } else {
            previousHotspotCenterX = hotspotCenterX
            previousHotspotCenterY = hotspotCenterY
            previousHotspotRadiusPixels = hotspotRadiusPixels
            hasPreviousHotspot = true
        }

        lastHotspotCenterX = hotspotCenterX
        lastHotspotCenterY = hotspotCenterY
        lastHotspotRadiusPixels = hotspotRadiusPixels
        ++validHotspotSampleCount
    }

    onImageRevisionChanged: updateHotspotHistory()

    Text {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        text: "Hotspot"
        color: "white"
        font.pixelSize: 16
        font.bold: true
        elide: Text.ElideRight
    }

    GridLayout {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        columns: 2
        columnSpacing: 18
        rowSpacing: 6

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: "Peak: "
                  + root.hotspotTemperatureText(
                      root.hotspotPeakTemperatureCelsius
                  )
                  + (root.hotspotPeakAboveRange ? " +" : "")
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: "Mean: "
                  + root.hotspotTemperatureText(
                      root.hotspotMeanTemperatureCelsius
                  )
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: "Radius: "
                  + (root.hotspotValid
                      ? root.hotspotRadiusPixels.toFixed(1) + " px"
                      : "n/a")
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: "Hot: "
                  + (root.hotspotValid
                      ? root.hotspotHotPixelCount
                        + " / "
                        + root.hotspotTotalPixelCount
                      : "n/a")
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: "Score: "
                  + (root.hotspotValid
                      ? root.hotspotScore.toFixed(1)
                      : "n/a")
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: "Center: "
                  + (root.hotspotValid
                      ? root.hotspotPointText(
                          root.hotspotCenterX,
                          root.hotspotCenterY
                      )
                      : "n/a")
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: "Previous: "
                  + (root.hotspotValid
                     && root.validHotspotSampleCount >= 2
                      ? root.hotspotPointText(
                          root.previousHotspotCenterX,
                          root.previousHotspotCenterY
                      )
                      : "n/a")
            color: "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: "Move: "
                  + (root.hotspotValid
                     && root.validHotspotSampleCount >= 2
                      ? root.hotspotMoveDistance.toFixed(2) + " px"
                      : "n/a")
                  + " - "
                  + root.hotspotMovementText
            color: root.hotspotMovementText === "Moved"
                   ? "#ffb86b"
                   : "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: "Size: " + root.hotspotSizeTrendText
            color: root.hotspotSizeTrendText === "Growing"
                   ? "#7fd88a"
                   : root.hotspotSizeTrendText === "Shrinking"
                     ? "#ff9f7a"
                     : "#d8d8df"
            font.pixelSize: 16
            elide: Text.ElideRight
        }
    }
}
