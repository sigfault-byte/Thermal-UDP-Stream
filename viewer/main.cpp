#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml>


#include "models/frame_model.h"
#include "network/udp_receiver.h"
#include "image/thermal_image_provider.h"
#include "analysis/frame_statistics_calculator.h"


int main(int argc, char *argv[])
{
    QGuiApplication padawan(argc, argv);

    FrameModel frameModel;
    UdpReceiver udpReceiver(5005);

    QQmlApplicationEngine engine;

    /*
     * The QML engine takes ownership of this provider.
     *
     * QML will access it using URLs beginning with:
     *
     *     image://thermal/
     */
    auto *thermalImageProvider =
        new ThermalImageProvider();

    engine.addImageProvider(
        "thermal",
        thermalImageProvider
    );

    /*
     * Whenever UdpReceiver decodes a valid thermal frame:
     *
     * 1. Update the provider's indexed image.
     * 2. Update the visible frame ID.
     * 3. Notify QML that it should request the image again.
     */
    QObject::connect(
        &udpReceiver,
        &UdpReceiver::thermalFrameReceived,
        &frameModel,
        [
            &frameModel,
            thermalImageProvider
        ](const ThermalFrame &frame)
        {
            const FrameStatistics statistics =
                FrameStatisticsCalculator::calculate(
                    frame.pixels
            );
            frameModel.setTimestampMs(
                frame.timestampMs
            );
            frameModel.setStatistics(
                statistics
            );

            thermalImageProvider->updateFrame(
                frame.pixels,
                statistics
            );

            thermalImageProvider->setScaleMode(
                frameModel.scaleMode()
            );

            frameModel.setFrameId(
                static_cast<int>(frame.frameId)
            );

            frameModel.notifyImageUpdated();
        }
    );

    engine.rootContext()->setContextProperty(
        "frameModel",
        &frameModel
    );

    // We do not want QML to construct. It is from C++ and expose in frameModel
    qmlRegisterUncreatableType<FrameModel>(
        "PadawanViewer",
        1,
        0,
        "FrameModel",
        "FrameModel is created in C++ and exposed as a context property."
    );

    engine.loadFromModule(
        "PadawanViewer",
        "Main"
    );

    return padawan.exec();
}

// int main(int argc, char *argv[])
// {
//     QGuiApplication app(argc, argv);

//     FrameModel frameModel;
//     // Listen for UDP datagrams on local port 5005.
//     UdpReceiver udpReceiver(5005);

//     QQmlApplicationEngine engine;

//     engine.rootContext()->setContextProperty(
//         "frameModel",
//         &frameModel
//     );

//     engine.loadFromModule(
//         "PadawanViewer",
//         "Main"
//     );

//     return app.exec();
// }
// int main(int argc, char *argv[])
// {
//     QGuiApplication app(argc, argv);

//     // This object owns the state that we want to expose to QML.
//     FrameModel frameModel;

//     QQmlApplicationEngine engine;

//     // Expose our C++ object to QML under the name "frameModel".
//     engine.rootContext()->setContextProperty(
//         "frameModel",
//         &frameModel
//     );

//     engine.loadFromModule("PadawanViewer", "Main");

//     // Temporary fake frame source.
//     // Later, the UDP receiver will update FrameModel instead.
//     QTimer timer;

//     QObject::connect(
//         &timer,
//         &QTimer::timeout,
//         [&frameModel]()
//         {
//             frameModel.setFrameId(
//                 frameModel.frameId() + 1
//             );
//         }
//     );

//     // Emit a timeout signal every second.
//     timer.start(1000);

//     return app.exec();
// }
