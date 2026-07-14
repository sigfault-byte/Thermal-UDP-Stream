#ifndef HANDSHAKE_RECEIVER_H
#define HANDSHAKE_RECEIVER_H

#include <QHostAddress>
#include <QObject>
#include <QUdpSocket>

class HandshakeReceiver : public QObject
{
    Q_OBJECT

public:
    // Create a receiver for Padawan discovery datagrams.
    // discoveryUdpPort is the local UDP port to listen on.
    // commandTcpPort is the fixed TCP port emitted with discovered camera IPs.
    explicit HandshakeReceiver(
        quint16 discoveryUdpPort,
        quint16 commandTcpPort,
        QObject *parent = nullptr
    );

signals:
    // Announce that a valid discovery packet identified a camera address.
    // The address comes from UDP sender metadata, not from packet bytes.
    void deviceDiscovered(
        const QHostAddress &address,
        quint16 tcpPort
    );

private slots:
    // Read and process every discovery datagram currently waiting on the socket.
    void processPendingDatagrams();

private:
    // The UDP socket used only for the small discovery broadcast packet.
    QUdpSocket m_socket;

    // The fixed TCP port that the command client will use later.
    quint16 m_commandTcpPort = 0;
};

#endif
