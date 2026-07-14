#include "command_packet.h"

namespace
{
constexpr qsizetype MagicSize = 8;
constexpr qsizetype VersionOffset = 8;
constexpr qsizetype CommandOffset = 9;
constexpr qsizetype ValueOrStatusOffset = 10;

constexpr quint8 SupportedVersion = 1;

// The fixed protocol signature includes the null byte after PADAWAN.
const QByteArray ExpectedMagic("PADAWAN\0", MagicSize);
}

QByteArray CommandPacket::buildRequest(
    quint8 command,
    quint8 value
)
{
    QByteArray packet;
    packet.resize(PacketSize);

    // Request layout:
    // bytes 0-7: "PADAWAN\0"
    // byte 8:    protocol version
    // byte 9:    command
    // byte 10:   command value, or 0 when unused.
    for (qsizetype index = 0; index < MagicSize; ++index)
    {
        packet[index] =
            ExpectedMagic.at(index);
    }

    packet[VersionOffset] =
        static_cast<char>(SupportedVersion);

    packet[CommandOffset] =
        static_cast<char>(command);

    packet[ValueOrStatusOffset] =
        static_cast<char>(value);

    return packet;
}

bool CommandPacket::parseResponse(
    const QByteArray &packet,
    CommandResponse &response,
    QString &errorMessage
)
{
    // Response packets are fixed-size so the socket buffer can parse them
    // one complete packet at a time.
    if (packet.size() != PacketSize)
    {
        errorMessage = QString(
            "Invalid command response size: expected %1 bytes, received %2"
        )
            .arg(PacketSize)
            .arg(packet.size());

        return false;
    }

    // Validate the exact shared Padawan signature before reading command bytes.
    if (packet.first(MagicSize) != ExpectedMagic)
    {
        errorMessage = "Invalid command response magic";
        return false;
    }

    const auto *bytes =
        reinterpret_cast<const uchar *>(packet.constData());

    const quint8 version =
        bytes[VersionOffset];

    if (version != SupportedVersion)
    {
        errorMessage = QString(
            "Unsupported command response version: %1"
        ).arg(version);

        return false;
    }

    const quint8 command =
        bytes[CommandOffset];

    if (!isSupportedCommand(command))
    {
        errorMessage = QString(
            "Unsupported echoed command: %1"
        ).arg(command);

        return false;
    }

    const quint8 status =
        bytes[ValueOrStatusOffset];

    if (!isSupportedStatus(status))
    {
        errorMessage = QString(
            "Unsupported command response status: %1"
        ).arg(status);

        return false;
    }

    response.command = command;
    response.status = status;

    errorMessage.clear();
    return true;
}

bool CommandPacket::parseExpectedResponse(
    const QByteArray &packet,
    quint8 expectedCommand,
    CommandResponse &response,
    QString &errorMessage
)
{
    // First parse the response packet itself.
    // Then confirm it acknowledges the command that is currently pending.
    if (
        !parseResponse(
            packet,
            response,
            errorMessage
        )
    )
    {
        return false;
    }

    if (response.command != expectedCommand)
    {
        errorMessage = QString(
            "Command response echoed %1 while waiting for %2"
        )
            .arg(commandName(response.command))
            .arg(commandName(expectedCommand));

        return false;
    }

    errorMessage.clear();
    return true;
}

bool CommandPacket::isSupportedCommand(
    quint8 command
)
{
    return command == StartCommand
        || command == StopCommand;
}

bool CommandPacket::isSupportedStatus(
    quint8 status
)
{
    return status == StatusOk
        || status == StatusError
        || status == StatusInvalidValue;
}

QString CommandPacket::commandName(
    quint8 command
)
{
    if (command == StartCommand)
    {
        return "START";
    }

    if (command == StopCommand)
    {
        return "STOP";
    }

    return QString("UNKNOWN(%1)")
        .arg(command);
}

QString CommandPacket::statusName(
    quint8 status
)
{
    if (status == StatusOk)
    {
        return "OK";
    }

    if (status == StatusError)
    {
        return "ERROR";
    }

    if (status == StatusInvalidValue)
    {
        return "INVALID_VALUE";
    }

    return QString("UNKNOWN(%1)")
        .arg(status);
}
