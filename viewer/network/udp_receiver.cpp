#include "udp_receiver.h"
#include "protocol/packet_decoder.h"

#include <QDebug>
#include <QHostAddress>
#include <QNetworkDatagram>

UdpReceiver::UdpReceiver(
    quint16 port,
    const QString &rawSavePath,
    bool bindSocket,
    QObject *parent
)
    : QObject(parent)
{
    if (!rawSavePath.isEmpty())
    {
        m_rawSaveFile.setFileName(rawSavePath);

        // Start a fresh capture file for this run.
        // The file contains only raw UDP payload bytes, one packet after another.
        if (!m_rawSaveFile.open(QIODevice::WriteOnly))
        {
            qCritical()
                << "Could not open raw packet capture file"
                << rawSavePath
                << ":"
                << m_rawSaveFile.errorString();
        }
        else
        {
            qInfo()
                << "Saving raw UDP payloads to"
                << rawSavePath;
        }
    }

    if (!bindSocket)
    {
        // Replay mode reads packets from a .bin file, so it must not also bind UDP.
        qInfo()
            << "UDP socket disabled because raw .bin replay is active";

        return;
    }

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

        processRawDatagram(
            payload
        );
    }
}

void UdpReceiver::processRawDatagram(
    const QByteArray &payload
)
{
    // The capture happens before validation because "raw" means keep what arrived.
    // This makes broken packets useful later when debugging the protocol.
    if (m_rawSaveFile.isOpen())
    {
        const qint64 written =
            m_rawSaveFile.write(payload);

        if (written != payload.size())
        {
            qWarning()
                << "Could not write full raw packet to capture file:"
                << m_rawSaveFile.errorString();
        }
    }

    // TODO: this only works if a datagram is always a complete packet.
    // If later the camera sends subpages, or if the viewer merges them,
    // this fixed one-payload-one-frame assumption will be wrong.
    m_statisticsTracker.registerDatagramReceived();

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

        return;
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

    // Send the meaningful decoded frame to the rest of the application.
    emit thermalFrameReceived(frame);
}
