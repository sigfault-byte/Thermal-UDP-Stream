#include "tcp_command_client.h"
#include "protocol/command_packet.h"
#include "protocol/thermal_quantization.h"

#include <QAbstractSocket>
#include <QDebug>

namespace
{
constexpr int CommandTimeoutMs = 3000;
}

TcpCommandClient::TcpCommandClient(
    QObject *parent
)
    : QObject(parent)
{
    // The command timeout is one-shot because each click sends exactly one
    // command and waits for exactly one ESP32 response.
    m_commandTimeoutTimer.setSingleShot(
        true
    );

    m_commandTimeoutTimer.setInterval(
        CommandTimeoutMs
    );

    // Once connected, write the pending command request bytes.
    QObject::connect(
        &m_socket,
        &QTcpSocket::connected,
        this,
        &TcpCommandClient::handleConnected
    );

    // TCP may deliver the 11-byte response in pieces, so readyRead appends to
    // an internal buffer before parsing complete response packets.
    QObject::connect(
        &m_socket,
        &QTcpSocket::readyRead,
        this,
        &TcpCommandClient::handleReadyRead
    );

    QObject::connect(
        &m_socket,
        &QTcpSocket::disconnected,
        this,
        &TcpCommandClient::handleDisconnected
    );

    QObject::connect(
        &m_socket,
        &QTcpSocket::errorOccurred,
        this,
        &TcpCommandClient::handleSocketError
    );

    QObject::connect(
        &m_commandTimeoutTimer,
        &QTimer::timeout,
        this,
        &TcpCommandClient::handleCommandTimeout
    );
}

void TcpCommandClient::setEndpoint(
    const QHostAddress &address,
    quint16 port
)
{
    // The ESP32 broadcasts the same discovery packet repeatedly.
    // If the endpoint did not actually change, do not notify QML or logs again.
    if (
        m_hasEndpoint
        && m_endpointAddress == address
        && m_endpointPort == port
    )
    {
        return;
    }

    // Store the prepared endpoint for the later TCP command socket slice.
    m_endpointAddress = address;
    m_endpointPort = port;
    m_hasEndpoint = true;

    // A new endpoint means any existing TCP connection belongs to the old IP.
    // Start fresh so commands go to the latest discovered camera.
    m_socket.abort();
    m_receiveBuffer.clear();

    if (m_commandPending)
    {
        m_commandPending = false;
        m_pendingCommand = 0;
        m_pendingCommandValue = 0;
        m_commandTimeoutTimer.stop();
    }

    emit endpointChanged();
    emit commandStateChanged();
}

bool TcpCommandClient::hasEndpoint() const
{
    return m_hasEndpoint;
}

QHostAddress TcpCommandClient::endpointAddress() const
{
    return m_endpointAddress;
}

QString TcpCommandClient::endpointAddressText() const
{
    // Empty text means discovery has not found a camera yet.
    if (!m_hasEndpoint)
    {
        return QString();
    }

    return m_endpointAddress.toString();
}

quint16 TcpCommandClient::endpointPort() const
{
    return m_endpointPort;
}

QString TcpCommandClient::endpointText() const
{
    // QML uses this as the compact "ip:port" status string.
    if (!m_hasEndpoint)
    {
        return QString();
    }

    return QString("%1:%2")
        .arg(m_endpointAddress.toString())
        .arg(m_endpointPort);
}

bool TcpCommandClient::canSendCommand() const
{
    // QML can send only after discovery and only when no command is pending.
    return m_hasEndpoint
        && !m_commandPending;
}

bool TcpCommandClient::commandPending() const
{
    return m_commandPending;
}

bool TcpCommandClient::cameraRunning() const
{
    return m_cameraRunning;
}

QString TcpCommandClient::commandButtonText() const
{
    if (m_commandPending)
    {
        return "WAIT";
    }

    if (m_cameraRunning)
    {
        return "STOP";
    }

    return "START";
}

QString TcpCommandClient::commandButtonColor() const
{
    if (!m_hasEndpoint || m_commandPending)
    {
        return "#777783";
    }

    return "#2f9e44";
}

void TcpCommandClient::sendStartCommand()
{
    sendCommand(
        CommandPacket::StartCommand,
        0
    );
}

void TcpCommandClient::sendStopCommand()
{
    sendCommand(
        CommandPacket::StopCommand,
        0
    );
}

void TcpCommandClient::sendSetQuantizationCommand(
    int mode
)
{
    if (mode < 0 || mode > 255)
    {
        qWarning()
            << "Cannot send unsupported quantization mode"
            << mode;

        return;
    }

    if (
        !ThermalQuantization::isValidMode(
            static_cast<quint8>(mode)
        )
    )
    {
        qWarning()
            << "Cannot send unsupported quantization mode"
            << mode;

        return;
    }

    sendCommand(
        CommandPacket::SetQuantizationCommand,
        static_cast<quint8>(mode)
    );
}

void TcpCommandClient::handleConnected()
{
    writePendingCommand();
}

void TcpCommandClient::handleReadyRead()
{
    m_receiveBuffer.append(
        m_socket.readAll()
    );

    while (
        m_commandPending
        && m_receiveBuffer.size() >= CommandPacket::PacketSize
    )
    {
        const QByteArray packet =
            m_receiveBuffer.left(
                CommandPacket::PacketSize
            );

        m_receiveBuffer.remove(
            0,
            CommandPacket::PacketSize
        );

        CommandResponse response;
        QString errorMessage;

        const bool parsed =
            CommandPacket::parseExpectedResponse(
                packet,
                m_pendingCommand,
                response,
                errorMessage
            );

        if (!parsed)
        {
            completePendingCommandFailure(
                errorMessage
            );

            return;
        }

        if (response.status != CommandPacket::StatusOk)
        {
            completePendingCommandFailure(
                QString(
                    "Command %1 failed with status %2"
                )
                    .arg(CommandPacket::commandName(response.command))
                    .arg(CommandPacket::statusName(response.status))
            );

            return;
        }

        completePendingCommandOk();
    }
}

void TcpCommandClient::handleDisconnected()
{
    if (!m_commandPending)
    {
        return;
    }

    completePendingCommandFailure(
        "TCP command socket disconnected before response"
    );
}

void TcpCommandClient::handleSocketError()
{
    if (!m_commandPending)
    {
        return;
    }

    completePendingCommandFailure(
        QString(
            "TCP command socket error: %1"
        ).arg(m_socket.errorString())
    );
}

void TcpCommandClient::handleCommandTimeout()
{
    if (!m_commandPending)
    {
        return;
    }

    completePendingCommandFailure(
        QString(
            "Timed out waiting %1 ms for command response"
        ).arg(CommandTimeoutMs)
    );
}

void TcpCommandClient::sendCommand(
    quint8 command,
    quint8 value
)
{
    if (!m_hasEndpoint)
    {
        qWarning()
            << "Cannot send command before TCP endpoint discovery";

        return;
    }

    if (m_commandPending)
    {
        qWarning()
            << "Cannot send command while another command is pending";

        return;
    }

    if (!CommandPacket::isSupportedCommand(command))
    {
        qWarning()
            << "Cannot send unsupported command"
            << command;

        return;
    }

    // Mark the UI pending before connecting or writing.
    // The button stays grey until the ESP32 confirms the command result.
    m_pendingCommand = command;
    m_pendingCommandValue = value;
    m_commandPending = true;
    m_receiveBuffer.clear();
    m_commandTimeoutTimer.start();

    emit commandStateChanged();

    if (m_socket.state() == QAbstractSocket::ConnectedState)
    {
        writePendingCommand();
        return;
    }

    if (m_socket.state() != QAbstractSocket::UnconnectedState)
    {
        m_socket.abort();
    }

    qInfo()
        << "Connecting TCP command socket to"
        << endpointText();

    m_socket.connectToHost(
        m_endpointAddress,
        m_endpointPort
    );
}

void TcpCommandClient::writePendingCommand()
{
    if (!m_commandPending)
    {
        return;
    }

    // START and STOP use value 0.
    // SET_QUANTIZATION uses value 1, 2, or 3.
    const QByteArray request =
        CommandPacket::buildRequest(
            m_pendingCommand,
            m_pendingCommandValue
        );

    const qint64 written =
        m_socket.write(
            request
        );

    if (written != request.size())
    {
        completePendingCommandFailure(
            QString(
                "Could not queue full command request: wrote %1 of %2 bytes"
            )
            .arg(written)
            .arg(request.size())
        );

        return;
    }

    qInfo()
        << "Sent TCP command"
        << CommandPacket::commandName(m_pendingCommand)
        << "value"
        << m_pendingCommandValue
        << "to"
        << endpointText();
}

void TcpCommandClient::completePendingCommandOk()
{
    // The running state changes only after the ESP32 sends an OK response.
    if (m_pendingCommand == CommandPacket::StartCommand)
    {
        m_cameraRunning = true;
    }
    else if (m_pendingCommand == CommandPacket::StopCommand)
    {
        m_cameraRunning = false;
    }
    else if (m_pendingCommand == CommandPacket::SetQuantizationCommand)
    {
        qInfo()
            << "ESP32 accepted quantization mode"
            << m_pendingCommandValue
            << "- waiting for matching UDP frame mode";
    }

    qInfo()
        << "TCP command acknowledged:"
        << CommandPacket::commandName(m_pendingCommand);

    m_pendingCommand = 0;
    m_pendingCommandValue = 0;
    m_commandPending = false;
    m_commandTimeoutTimer.stop();

    emit commandStateChanged();
}

void TcpCommandClient::completePendingCommandFailure(
    const QString &message
)
{
    qWarning()
        << "TCP command failed:"
        << message;

    // Leave m_cameraRunning unchanged because the ESP32 did not confirm success.
    m_pendingCommand = 0;
    m_pendingCommandValue = 0;
    m_commandPending = false;
    m_commandTimeoutTimer.stop();
    m_receiveBuffer.clear();

    emit commandStateChanged();
}
