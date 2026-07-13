import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root

    required property bool rolling
    signal modeRequested(bool rolling)

    spacing: 10

    Text {
        text: "Raw"
        color: root.rolling ? "#777783" : "white"
        font.pixelSize: 16
    }

    Switch {
        id: modeSwitch
        // reflect the state supplied by the parent
        checked: root.rolling

        onToggled: {
            // Tell the parent what the user requested.
            // never modify FrameModel directly.
            root.modeRequested(checked)
        }
    }

    Text {
        text: "Rolling"
        color: root.rolling ? "white" : "#777783"
        font.pixelSize: 16
    }
}
