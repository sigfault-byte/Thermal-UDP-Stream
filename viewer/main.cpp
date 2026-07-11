#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>


#include "models/frame_model.h"
#include "network/udp_receiver.h"


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    FrameModel frameModel;

    // Listen for UDP datagrams on local port 5005.
    UdpReceiver udpReceiver(5005);

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(
        "frameModel",
        &frameModel
    );

    engine.loadFromModule(
        "PadawanViewer",
        "Main"
    );

    return app.exec();
}
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
