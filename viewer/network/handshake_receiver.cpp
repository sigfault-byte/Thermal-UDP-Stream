#include "handshake_receiver.h"
#include "protocol/handshake_packet.h"

#include <QDebug>
#include <QNetworkDatagram>

HandshakeReceiver::HandshakeReceiver(
    quint16 discoveryUdpPort,
    quint16 commandTcpPort,
    QObject *parent
)
    : QObject(parent),
      m_commandTcpPort(commandTcpPort)
{
    // Connect readyRead before binding so no pending datagrams are missed
    // after the operating system starts delivering them to this socket.
    QObject::connect(
        &m_socket,
        &QUdpSocket::readyRead,
        this,
        &HandshakeReceiver::processPendingDatagrams
    );

    // ShareAddress and ReuseAddressHint make repeated local development runs
    // less likely to fail if the OS still has the port in a reusable state.
    const bool bindSucceeded =
        m_socket.bind(
            QHostAddress::AnyIPv4,
            discoveryUdpPort,
            QUdpSocket::ShareAddress
                | QUdpSocket::ReuseAddressHint
        );

    if (!bindSucceeded)
    {
        qWarning()
            << "Could not bind discovery UDP port"
            << discoveryUdpPort
            << "socket error"
            << m_socket.error()
            << ":"
            << m_socket.errorString();

        return;
    }

    qInfo()
        << "Listening for Padawan discovery on UDP port"
        << discoveryUdpPort;
}

void HandshakeReceiver::processPendingDatagrams()
{
    while (m_socket.hasPendingDatagrams())
    {
        const QNetworkDatagram datagram =
            m_socket.receiveDatagram();

        if (!datagram.isValid())
        {
            qWarning()
                << "Could not read discovery datagram:"
                << m_socket.errorString();

            continue;
        }

        const QByteArray payload =
            datagram.data();

        QString errorMessage;

        const bool isValidDiscovery =
            HandshakePacket::decodeAndValidate(
                payload,
                errorMessage
            );

        if (!isValidDiscovery)
        {
            // Random traffic on the discovery UDP port is expected on real LANs.
            // Keep this quiet unless debug logging is enabled.
            qDebug()
                << "Ignoring discovery datagram:"
                << errorMessage;

            continue;
        }

        // The sender address is the camera IP.
        // The discovery packet itself contains no IP address and no TCP port.
        emit deviceDiscovered(
            datagram.senderAddress(),
            m_commandTcpPort
        );
    }
}
