#include "udp_receiver.h"
#include "protocol/packet_decoder.h"

#include <QDebug>
#include <QHostAddress>
#include <QNetworkDatagram>

UdpReceiver::UdpReceiver(
    quint16 port,
    QObject *parent
)
    : QObject(parent)
{
    // Connect this socket's readyRead signal to our datagram reader.
    QObject::connect(
        &m_socket,
        &QUdpSocket::readyRead,
        this,
        &UdpReceiver::processPendingDatagrams
    );

    // Ask the operating system to deliver UDP datagrams arriving
    // on this local port to m_socket.
    const bool bindSucceeded = m_socket.bind(
        //Accept packets sent to any IPv4 address
        // Someone simialr to 0.0.0.0 with python requests
        QHostAddress::AnyIPv4,
        port
    );

    if (!bindSucceeded)
    {
        qCritical()
            << "Could not bind UDP port"
            << port
            << ":"
            << m_socket.errorString();

        return;
    }

    qInfo()
        << "Listening for UDP datagrams on port"
        << port;
}

void UdpReceiver::processPendingDatagrams()
{
    while (m_socket.hasPendingDatagrams())
    {
        const QNetworkDatagram datagram =
            m_socket.receiveDatagram();

        //TODO: this only works if a datagram is always a complete packet.
        // If later the camera sends subpages, or if the viewer merges them
        // this will be wrong
        m_statisticsTracker.registerDatagramReceived();

        const QByteArray payload =
            datagram.data();

        ThermalFrame frame;
        QString errorMessage;

        const bool decoded =
            PacketDecoder::decodeThermalFrame(
                payload,
                frame,
                errorMessage
            );

        if (!decoded)
        {
            m_statisticsTracker.registerDatagramRejected();

            emit receiverStatisticsChanged(
                m_statisticsTracker.statistics()
            );

            qWarning()
                << "Rejected datagram:"
                << errorMessage;

            continue;
        }

        m_statisticsTracker.registerCompletedFrame(
                frame.frameId
        );
        emit receiverStatisticsChanged(
            m_statisticsTracker.statistics()
        );

        qInfo()
            << "Decoded thermal frame"
            << frame.frameId
            << "timestamp"
            << frame.timestampMs
            << "pixels"
            << frame.pixels.size();
        // send the meaningful decoded frame to the rest of the application
        emit thermalFrameReceived(frame);
    }
}
