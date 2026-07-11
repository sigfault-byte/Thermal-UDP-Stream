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
            qWarning()
                << "Rejected datagram:"
                << errorMessage;

            continue;
        }

        qInfo()
            << "Decoded thermal frame"
            << frame.frameId
            << "timestamp"
            << frame.timestampMs
            << "pixels"
            << frame.pixels.size();
    }
}

// void UdpReceiver::processPendingDatagrams()
// {
//     // readyRead means at least one datagram is available.
//     // Several datagrams may have arrived before this function runs,
//     // so consume all pending datagrams.
//     while (m_socket.hasPendingDatagrams())
//     {
//         // How can this be a const?
//         //
//         const QNetworkDatagram datagram =
//             // because m_socket is a QUdpSocket, it has the following method
//             // UdpReceiver uses a UDP socket as an implementation detail
//             m_socket.receiveDatagram();

//         const QByteArray payload = datagram.data();

//         qInfo()
//             << "Received"
//             << payload.size()
//             << "bytes from"
//             << datagram.senderAddress().toString()
//             << ":"
//             << datagram.senderPort();

//         // Print the payload as hexadecimal bytes.
//         qInfo()
//             << "Payload:"
//             << payload.toHex(' ');
//     }
// }
