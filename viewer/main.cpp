#include <QGuiApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QtQml>

#include <QDebug>

#include "analysis/frame_statistics_calculator.h"
#include "analysis/frame_timing_tracker.h"
#include "detector/hotspot_settings.h"
#include "image/thermal_image_provider.h"
#include "models/frame_model.h"
#include "network/udp_receiver.h"
#include "protocol/packet_decoder.h"
#include "detector/hotspot_detector.h"
#include "detector/hotspot_settings.h"
#include "timeseries/time_series_recorder.h"

namespace
{
constexpr quint16 ReceiverUdpPort = 5005;
}

int main(int argc, char *argv[])
{
    // Creates the Qt GUI application object.
    // This owns the main event loop used by Qt, QML, timers, sockets, signals, etc.
    QGuiApplication padawan(argc, argv);

    QCommandLineParser commandLineParser;
    commandLineParser.setApplicationDescription(
        "Padawan thermal camera viewer"
    );
    commandLineParser.addHelpOption();

    // Save mode copies every UDP payload into a raw .bin file.
    // The file has no header: it is only 786-byte packets back-to-back.
    const QCommandLineOption saveBinOption(
        QStringList() << "save-bin",
        "Save raw UDP payloads to <path>.",
        "path"
    );

    // Read mode replays that same raw .bin format into the normal app pipeline.
    // UDP is disabled in this mode, because bytes are coming from the file.
    const QCommandLineOption readBinOption(
        QStringList() << "read-bin",
        "Read raw UDP payloads from <path> instead of listening on UDP.",
        "path"
    );

    // Replay speed is a simple timer delay, not frames-per-second math.
    // 1000 ms means one packet per second; 0 means Qt runs the timer quickly.
    const QCommandLineOption speedOption(
        QStringList() << "speed",
        "Raw .bin replay delay in milliseconds between packets. Defaults to 1000.",
        "ms",
        "1000"
    );

    commandLineParser.addOption(
        saveBinOption
    );
    commandLineParser.addOption(
        readBinOption
    );
    commandLineParser.addOption(
        speedOption
    );
    commandLineParser.process(
        padawan
    );

    const QString saveBinPath =
        commandLineParser.value(saveBinOption);
    const QString readBinPath =
        commandLineParser.value(readBinOption);

    bool speedParsed = false;
    const int rawReplayIntervalMs =
        commandLineParser.value(speedOption).toInt(
            &speedParsed
        );

    if (!speedParsed || rawReplayIntervalMs < 0)
    {
        qCritical()
            << "--speed must be a non-negative integer number of milliseconds";

        return 1;
    }

    if (!saveBinPath.isEmpty() && !readBinPath.isEmpty())
    {
        qCritical()
            << "--save-bin and --read-bin cannot be used together";

        return 1;
    }

    const bool readBinMode =
        !readBinPath.isEmpty();

    // Backend state object exposed to QML.
    // QML reads properties from this object and reacts to its NOTIFY signals.
    FrameModel frameModel;
    FrameTimingTracker frameTimingTracker;
    QTimer frameTimingRefreshTimer;
    QTimer rawReplayTimer;
    QFile rawReplayFile;

    //time serie test
    TimeSeriesRecorder timeSeriesRecorder;
    timeSeriesRecorder.start();


    // Backend network object.
    // It listens for UDP packets and emits thermalFrameReceived(...) when a valid
    // thermal frame has been decoded.
    //
    // In .bin replay mode the object is still used, but the socket is not bound.
    // That keeps live UDP and replay going through the same raw-packet decoder.
    UdpReceiver udpReceiver(
        ReceiverUdpPort,
        saveBinPath,
        !readBinMode
    );

    if (readBinMode)
    {
        rawReplayFile.setFileName(
            readBinPath
        );

        if (!rawReplayFile.open(QIODevice::ReadOnly))
        {
            qCritical()
                << "Could not open raw packet replay file"
                << readBinPath
                << ":"
                << rawReplayFile.errorString();

            return 1;
        }

        qInfo()
            << "Reading raw packets from"
            << readBinPath;
    }

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

    engine.rootContext()->setContextProperty(
        "timeSeriesRecorder",
        &timeSeriesRecorder
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
    // 4. hotspotSettings : default settings for the hotspot.
    QObject::connect(
        &udpReceiver,
        &UdpReceiver::thermalFrameReceived,
        &frameModel,
        [
            &frameModel,
            &frameTimingTracker,
            &timeSeriesRecorder,
            thermalImageProvider
        ](const ThermalFrame &frame)
        {
            const FrameTimingStatistics timingStatistics =
                frameTimingTracker.registerReceivedFrame(
                    frame.timestampMs
                );

            const Hotspot hotspot =
                HotspotDetector::detect(frame.pixels, frameModel.hotspotSettings());

            // Compute min/max/other statistics from the raw pixel bytes.
            const FrameStatistics statistics =
                FrameStatisticsCalculator::calculate(
                    frame.pixels
                );

            timeSeriesRecorder.append(
                frame.timestampMs,
                statistics,
                hotspot
            );

            // Update QML-visible metadata.
            frameModel.setTimestampMs(
                frame.timestampMs
            );

            frameModel.setHotspot(hotspot);

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

    if (readBinMode)
    {
        rawReplayTimer.setInterval(
            rawReplayIntervalMs
        );

        QObject::connect(
            &rawReplayTimer,
            &QTimer::timeout,
            &udpReceiver,
            [
                &rawReplayFile,
                &rawReplayTimer,
                &udpReceiver
            ]()
            {
                // Raw .bin replay is intentionally simple:
                // each tick reads exactly one original UDP payload.
                const QByteArray payload =
                    rawReplayFile.read(
                        PacketDecoder::RawThermalPacketSize
                    );

                if (payload.isEmpty())
                {
                    qInfo()
                        << "Finished raw packet replay";

                    rawReplayTimer.stop();
                    return;
                }

                if (payload.size() != PacketDecoder::RawThermalPacketSize)
                {
                    qWarning()
                        << "Ignoring trailing bytes in raw packet replay file:"
                        << payload.size()
                        << "bytes";

                    rawReplayTimer.stop();
                    return;
                }

                udpReceiver.processRawDatagram(
                    payload
                );
            }
        );
    }

    // Load the QML UI.
    //
    // The C++ objects should be exposed before this call, because QML may access
    // frameModel immediately while Main.qml is being created.
    engine.loadFromModule(
        "PadawanViewer",
        "Main"
    );

    if (readBinMode)
    {
        // Start replay after QML is loaded, so the first decoded frame has a UI waiting.
        rawReplayTimer.start();
    }

    // Start the Qt event loop.
    // From here on, signals, sockets, QML bindings, rendering, etc. happen.
    return padawan.exec();
}
