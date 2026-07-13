#ifndef PACKET_DECODER_H
#define PACKET_DECODER_H

#include <QByteArray>
#include <QString>

#include "thermal_frame.h"

// Special class to verify the raw datagram

class PacketDecoder
{

// This is declared as static: the function belongs to the class scope
// but does not require a PacketDecoder object
// So later call PacketDecoder::decodeThermalFrame(...);
// without constructing PacketDecoder decoder; ( that doesnt even exist :D)
public:
    // A raw .bin file is just these UDP payloads glued together.
    // There is no extra file header, so replay reads exactly this many bytes.
    static constexpr qsizetype RawThermalPacketSize =
        18 + (32 * 24);

    // decode one complete UDP datagram into a ThermalFrame.
    // returns true when decoding succeeds.
    // returns false and fills errorMessage when validation fails.
    static bool decodeThermalFrame(
        // receives the raw datagram array by reference, but won t edit it.
        // why copy when you can pass the reference itself lovely c logic
        const QByteArray &datagram,
        // directly write the important part to the thermalFrame struct
        ThermalFrame &frame,
        // special Q string for error messages.
        QString &errorMessage
    );
};

#endif
