import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtGraphs

Rectangle {
    id: root

    color: "#18181f"
    radius: 8

    required property real quantizationMinimumCelsius
    required property real quantizationMaximumCelsius

    property bool autoScaleTemperatureAxis: false
    property real recordedMinimumCelsius: 0
    property real recordedMaximumCelsius: 0
    property bool hasRecordedTemperatureRange: false
    readonly property color meanSeriesColor: "#55c7ff"
    readonly property color hotspotSeriesColor: "#ff6b4a"

    function resetRecordedTemperatureRange() {
        recordedMinimumCelsius = 0
        recordedMaximumCelsius = 0
        hasRecordedTemperatureRange = false
    }

    function includeRecordedTemperatureRange(
        minimumCelsius,
        maximumCelsius
    ) {
        if (!hasRecordedTemperatureRange) {
            recordedMinimumCelsius = minimumCelsius
            recordedMaximumCelsius = maximumCelsius
            hasRecordedTemperatureRange = true
            return
        }

        recordedMinimumCelsius = Math.min(
            recordedMinimumCelsius,
            minimumCelsius
        )

        recordedMaximumCelsius = Math.max(
            recordedMaximumCelsius,
            maximumCelsius
        )
    }

    function updateTemperatureAxis() {
        if (
            !autoScaleTemperatureAxis
            || !hasRecordedTemperatureRange
        ) {
            temperatureAxis.min = root.quantizationMinimumCelsius
            temperatureAxis.max = root.quantizationMaximumCelsius
            return
        }

        const range = recordedMaximumCelsius - recordedMinimumCelsius
        const padding = Math.max(
            0.5,
            range * 0.08
        )

        temperatureAxis.min = recordedMinimumCelsius - padding
        temperatureAxis.max = recordedMaximumCelsius + padding
    }

    onQuantizationMinimumCelsiusChanged: updateTemperatureAxis()
    onQuantizationMaximumCelsiusChanged: updateTemperatureAxis()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text:
                    timeSeriesRecorder.recording
                    ? "Stop recording"
                    : "Start recording"

                onClicked: {
                    if (timeSeriesRecorder.recording)
                        timeSeriesRecorder.stop()
                    else
                        timeSeriesRecorder.start()
                }
            }

            Button {
                text: "Clear"

                onClicked:
                    timeSeriesRecorder.clear()
            }

            Button {
                text:
                    root.autoScaleTemperatureAxis
                    ? "Fixed scale"
                    : "Auto scale"

                onClicked: {
                    root.autoScaleTemperatureAxis =
                        !root.autoScaleTemperatureAxis

                    root.updateTemperatureAxis()
                }
            }

            RowLayout {
                spacing: 10

                RowLayout {
                    spacing: 4

                    Rectangle {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 3
                        color: root.meanSeriesColor
                    }

                    Text {
                        text: "Mean"
                        color: "#d8d8df"
                        font.pixelSize: 12
                    }
                }

                RowLayout {
                    spacing: 4

                    Rectangle {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 3
                        color: root.hotspotSeriesColor
                    }

                    Text {
                        text: "Hotspot"
                        color: "#d8d8df"
                        font.pixelSize: 12
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        GraphsView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0

            axisX: ValueAxis {
                id: timeAxis

                min: 0
                max: 30
                tickInterval: 5
                labelDecimals: 0
            }

            axisY: ValueAxis {
                id: temperatureAxis

                min: root.quantizationMinimumCelsius
                max: root.quantizationMaximumCelsius
                tickInterval: 5
                labelDecimals: 0
            }

            LineSeries {
                id: meanSeries
                width: 3
                color: root.meanSeriesColor
            }

            LineSeries {
                id: hotspotSeries
                width: 3
                color: root.hotspotSeriesColor
            }

            Connections {
                target: timeSeriesRecorder

                function onSampleCountChanged() {
                    if (timeSeriesRecorder.sampleCount === 0) {
                        meanSeries.clear()
                        hotspotSeries.clear()
                        root.resetRecordedTemperatureRange()
                        root.updateTemperatureAxis()
                    }
                }

                function onSampleAppended(
                    timestampMs,
                    minimumCelsius,
                    maximumCelsius,
                    meanCelsius,
                    hotspotValid,
                    hotspotPeakTemperatureCelsius
                ) {
                    const seconds = timestampMs / 1000.0

                    meanSeries.append(
                        seconds,
                        meanCelsius
                    )

                    if (hotspotValid) {
                        hotspotSeries.append(
                            seconds,
                            hotspotPeakTemperatureCelsius
                        )
                    }

                    root.includeRecordedTemperatureRange(
                        minimumCelsius,
                        maximumCelsius
                    )

                    if (hotspotValid) {
                        root.includeRecordedTemperatureRange(
                            hotspotPeakTemperatureCelsius,
                            hotspotPeakTemperatureCelsius
                        )
                    }

                    root.updateTemperatureAxis()

                    timeAxis.max = Math.max(
                        30,
                        seconds
                    )

                    timeAxis.min = Math.max(
                        0,
                        timeAxis.max - 30
                    )
                }
            }
        }
    }

}
