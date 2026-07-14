#ifndef COMMAND_PACKET_H
#define COMMAND_PACKET_H

#include <QByteArray>
#include <QString>

struct CommandResponse
{
    // The command byte echoed by the ESP32 response packet.
    quint8 command = 0;

    // The response status byte returned by the ESP32.
    quint8 status = 0;
};

class CommandPacket
{
public:
    // The TCP command request and response packets are both exactly 11 bytes.
    static constexpr qsizetype PacketSize = 11;

    // Command byte values shared with the ESP32 firmware.
    static constexpr quint8 StartCommand = 1;
    static constexpr quint8 StopCommand = 2;

    // Response status byte values shared with the ESP32 firmware.
    static constexpr quint8 StatusOk = 1;
    static constexpr quint8 StatusError = 2;
    static constexpr quint8 StatusInvalidValue = 3;

    // Build one request packet:
    // magic[8], version, command, value.
    static QByteArray buildRequest(
        quint8 command,
        quint8 value
    );

    // Parse one response packet:
    // magic[8], version, echoed command, status.
    static bool parseResponse(
        const QByteArray &packet,
        CommandResponse &response,
        QString &errorMessage
    );

    // Parse a response and require it to echo the command currently pending.
    static bool parseExpectedResponse(
        const QByteArray &packet,
        quint8 expectedCommand,
        CommandResponse &response,
        QString &errorMessage
    );

    static bool isSupportedCommand(
        quint8 command
    );

    static bool isSupportedStatus(
        quint8 status
    );

    static QString commandName(
        quint8 command
    );

    static QString statusName(
        quint8 status
    );
};

#endif
