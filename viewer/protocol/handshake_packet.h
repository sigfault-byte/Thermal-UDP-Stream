#ifndef HANDSHAKE_PACKET_H
#define HANDSHAKE_PACKET_H

#include <QByteArray>
#include <QString>

// Helper for the small Padawan discovery handshake packet.
//
// This class does not use a packed C struct on purpose.
// The packet is only 10 bytes, so reading explicit byte positions is clearer
// and avoids compiler padding or alignment surprises.
class HandshakePacket
{
public:
    // The discovery packet is exactly:
    // 8 magic bytes + 1 version byte + 1 packet-type byte.
    static constexpr qsizetype EncodedPacketSize = 10;

    // Decode the repeating-key XOR used by the ESP32 broadcast.
    // XOR is reversible, so the same operation would encode the bytes again.
    static QByteArray decodeXor(
        const QByteArray &encodedPacket
    );

    // Validate one already-decoded discovery packet.
    // Returns false and fills errorMessage when the bytes are not Padawan discovery.
    static bool validateDecoded(
        const QByteArray &decodedPacket,
        QString &errorMessage
    );

    // Decode and validate one encoded discovery packet.
    // This is the normal entry point used by the UDP discovery receiver.
    static bool decodeAndValidate(
        const QByteArray &encodedPacket,
        QString &errorMessage
    );
};

#endif
