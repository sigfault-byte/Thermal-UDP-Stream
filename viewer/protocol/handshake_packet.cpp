#include "handshake_packet.h"

#include <array>

namespace
{
constexpr qsizetype MagicSize = 8;
constexpr qsizetype VersionOffset = 8;
constexpr qsizetype PacketTypeOffset = 9;

constexpr quint8 SupportedVersion = 0x01;
constexpr quint8 DiscoveryPacketType = 0x01;

// This key is exactly seven lowercase ASCII bytes.
// It intentionally does not include a trailing '\0' byte.
inline constexpr std::array<quint8, 7> HandshakeXorKey = {
    'p',
    'a',
    'd',
    'a',
    'w',
    'a',
    'n'
};

// The fixed decoded signature is eight bytes:
// P A D A W A N and then one null byte.
const QByteArray ExpectedMagic("PADAWAN\0", MagicSize);
}

QByteArray HandshakePacket::decodeXor(
    const QByteArray &encodedPacket
)
{
    QByteArray decodedPacket =
        encodedPacket;

    // Apply the lowercase key repeatedly across every packet byte.
    // This keeps the operation independent from C-string null termination.
    for (qsizetype index = 0; index < decodedPacket.size(); ++index)
    {
        const quint8 encodedByte =
            static_cast<quint8>(decodedPacket.at(index));

        const quint8 keyByte =
            HandshakeXorKey[
                static_cast<std::size_t>(index)
                    % HandshakeXorKey.size()
            ];

        decodedPacket[index] =
            static_cast<char>(
                encodedByte ^ keyByte
            );
    }

    return decodedPacket;
}

bool HandshakePacket::validateDecoded(
    const QByteArray &decodedPacket,
    QString &errorMessage
)
{
    // Discovery packets are fixed size.
    // Anything else is unrelated UDP traffic or a malformed test packet.
    if (decodedPacket.size() != EncodedPacketSize)
    {
        errorMessage = QString(
            "Invalid handshake size: expected %1 bytes, received %2"
        )
            .arg(EncodedPacketSize)
            .arg(decodedPacket.size());

        return false;
    }

    // Validate the exact eight-byte Padawan discovery magic.
    // The eighth byte must be '\0', not a missing string terminator.
    if (decodedPacket.first(MagicSize) != ExpectedMagic)
    {
        errorMessage = "Invalid handshake magic";
        return false;
    }

    const auto *bytes =
        reinterpret_cast<const uchar *>(decodedPacket.constData());

    const quint8 version =
        bytes[VersionOffset];

    const quint8 packetType =
        bytes[PacketTypeOffset];

    // Version 1 is the only discovery protocol version supported by this slice.
    if (version != SupportedVersion)
    {
        errorMessage = QString(
            "Unsupported handshake version: %1"
        ).arg(version);

        return false;
    }

    // Type 1 means ESP32 camera discovery.
    // No IP address or port is stored inside this packet.
    if (packetType != DiscoveryPacketType)
    {
        errorMessage = QString(
            "Unexpected handshake packet type: %1"
        ).arg(packetType);

        return false;
    }

    errorMessage.clear();
    return true;
}

bool HandshakePacket::decodeAndValidate(
    const QByteArray &encodedPacket,
    QString &errorMessage
)
{
    // Decode first, then validate the plain protocol bytes.
    // Keeping the steps separate makes test cases easier to understand.
    const QByteArray decodedPacket =
        decodeXor(
            encodedPacket
        );

    return validateDecoded(
        decodedPacket,
        errorMessage
    );
}
