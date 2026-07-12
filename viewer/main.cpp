#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QtQml>

#include "analysis/frame_statistics_calculator.h"
#include "analysis/frame_timing_tracker.h"
#include "image/thermal_image_provider.h"
#include "models/frame_model.h"
#include "network/udp_receiver.h"

namespace
{
constexpr quint16 ReceiverUdpPort = 5005;
}

int main(int argc, char *argv[])
{
    // Creates the Qt GUI application object.
    // This owns the main event loop used by Qt, QML, timers, sockets, signals, etc.
    QGuiApplication padawan(argc, argv);

    // Backend state object exposed to QML.
    // QML reads properties from this object and reacts to its NOTIFY signals.
    FrameModel frameModel;
    FrameTimingTracker frameTimingTracker;
    QTimer frameTimingRefreshTimer;

    // Backend network object.
    // It listens for UDP packets and emits thermalFrameReceived(...) when a valid
    // thermal frame has been decoded.
    UdpReceiver udpReceiver(ReceiverUdpPort);

    // QML engine.
    // This loads the QML module and creates the QML object tree.
    QQmlApplicationEngine engine;

    // Image provider used by QML Image elements.
    //
    // QML will request images from it using URLs like:
    //
    //     image://thermal/something
    //
    // The "thermal" name below is the provider ID.
    //
    // Important: after addImageProvider(), the QML engine owns this pointer.
    auto *thermalImageProvider =
        new ThermalImageProvider();

    engine.addImageProvider(
        "thermal",
        thermalImageProvider
    );

    // Expose the existing C++ FrameModel instance to QML.
    //
    // After this, QML can refer to this exact object as:
    //
    //     frameModel
    //
    // Important: setContextProperty() does NOT give ownership to QML.
    // frameModel still lives here on the C++ stack, so it must outlive the QML UI.
    engine.rootContext()->setContextProperty(
        "frameModel",
        &frameModel
    );

    engine.rootContext()->setContextProperty(
        "receiverUdpPort",
        ReceiverUdpPort
    );

    // Register the FrameModel C++ type name with QML.
    //
    // This does not expose the frameModel object.
    // The object exposure already happened above with setContextProperty().
    //
    // This only tells QML about the FrameModel type, for example so QML can
    // understand enum values or refer to the type name.
    //
    // "Uncreatable" means QML is NOT allowed to do:
    //
    //     FrameModel {}
    //
    // because the real FrameModel instance is created in C++.
    qmlRegisterUncreatableType<FrameModel>(
        "PadawanViewer",
        1,
        0,
        "FrameModel",
        "FrameModel is created in C++ and exposed as a context property."
    );

    // Connect the backend data flow.
    //
    // When udpReceiver emits thermalFrameReceived(frame), this function updates:
    //
    // 1. FrameModel: metadata and statistics visible to QML.
    // 2. ThermalImageProvider: actual image pixels used by image://thermal.
    // 3. FrameModel image revision: tells QML that the image should be requested again.
    QObject::connect(
        &udpReceiver,
        &UdpReceiver::thermalFrameReceived,
        &frameModel,
        [
            &frameModel,
            &frameTimingTracker,
            thermalImageProvider
        ](const ThermalFrame &frame)
        {
            const FrameTimingStatistics timingStatistics =
                frameTimingTracker.registerReceivedFrame(
                    frame.timestampMs
                );

            // Compute min/max/other statistics from the raw pixel bytes.
            const FrameStatistics statistics =
                FrameStatisticsCalculator::calculate(
                    frame.pixels
                );

            // Update QML-visible metadata.
            frameModel.setTimestampMs(
                frame.timestampMs
            );

            frameModel.setFrameTimingStatistics(
                timingStatistics
            );

            frameModel.setStatistics(
                statistics
            );

            frameModel.setFrameId(
                static_cast<int>(frame.frameId)
            );

            // Update the image provider's internal QImage.
            //
            // QML does not receive this image directly from the signal.
            // QML will later request it through image://thermal/...
            thermalImageProvider->updateFrame(
                frame.pixels,
                statistics
            );

            // Keep the provider's display scaling mode synchronized with the model.
            thermalImageProvider->setScaleMode(
                frameModel.scaleMode()
            );

            // Increment the image revision and emit imageRevisionChanged().
            //
            // This gives QML a property change to react to, so it can refresh
            // the image://thermal source.
            frameModel.notifyImageUpdated();
        }
    );

    frameTimingRefreshTimer.setInterval(
        250
    );

    QObject::connect(
        &frameTimingRefreshTimer,
        &QTimer::timeout,
        &frameModel,
        [
            &frameModel,
            &frameTimingTracker
        ]()
        {
            frameModel.setFrameTimingStatistics(
                frameTimingTracker.refresh()
            );
        }
    );

    frameTimingRefreshTimer.start();

    // Receiver/network statistics also flow through FrameModel,
    // so QML only needs one backend object.
    QObject::connect(
        &udpReceiver,
        &UdpReceiver::receiverStatisticsChanged,
        &frameModel,
        &FrameModel::setReceiverStatistics
    );

    // Load the QML UI.
    //
    // The C++ objects should be exposed before this call, because QML may access
    // frameModel immediately while Main.qml is being created.
    engine.loadFromModule(
        "PadawanViewer",
        "Main"
    );

    // Start the Qt event loop.
    // From here on, signals, sockets, QML bindings, rendering, etc. happen.
    return padawan.exec();
}
