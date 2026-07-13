#include "packet_decoder.h"
#include "detector/hotspot_detector.h"
// #include "thermal_quantization.h"

#include <QtEndian>
// #include <QDebug>

// Ah yes namespace macro?
// nope, this is an anonymous real namespace
// it is like in C with static const int MAGIC_SIZE = 8;
// It only exist within this translation unit during compialtion / execution
namespace
{
    constexpr qsizetype MagicSize = 8;
    constexpr qsizetype HeaderSize = 18;
    constexpr qsizetype PixelCount = 32 * 24;

    constexpr quint8 SupportedVersion = 1;
    constexpr quint8 ThermalFrameType = 1;

    // const QByteArray ExpectedMagic("PADAWAN\0", MagicSize);
    const QByteArray ExpectedMagic("PADAWAN", MagicSize);
}

bool PacketDecoder::decodeThermalFrame(
    const QByteArray &datagram,
    ThermalFrame &frame,
    QString &errorMessage
)
{
    // A thermal packet must have exactly one header and 768 pixels.
    if (datagram.size() != PacketDecoder::RawThermalPacketSize)
    {
        errorMessage = QString(
            "Invalid packet size: expected %1 bytes, received %2"
        )
            .arg(PacketDecoder::RawThermalPacketSize)
            .arg(datagram.size());

        return false;
    }

    // Validate the fixed eight-byte protocol signature.
    if (datagram.first(MagicSize) != ExpectedMagic)
    {
        errorMessage = "Invalid protocol magic";
        return false;
    }
    // auto means: compiler please infer the type from the expression on the right
    // uchar is qt's unsigned char
    const auto *bytes =
        reinterpret_cast<const uchar *>(datagram.constData());

    const quint8 version = bytes[8];
    const quint8 type = bytes[9]; // same as *(bytes + 9), i guess i prefer the pointer version, i am too confuse from python

    if (version != SupportedVersion)
    {
        errorMessage = QString(
            "Unsupported protocol version: %1"
        ).arg(version);

        return false;
    }

    if (type != ThermalFrameType)
    {
        errorMessage = QString(
            "Unexpected packet type: %1"
        ).arg(type);

        return false;
    }

    // While Network Byte Order is traditionally BigEndian, standardizing on
    // Llittlendian eliminates byte-swapping overhead on this specific hardware
    // Apparently this is bad, but i am a bit confused for about ti for now
    frame.frameId =
        qFromLittleEndian<quint32>(bytes + 10);

    frame.timestampMs =
        qFromLittleEndian<quint32>(bytes + 14);

    frame.pixels =
        datagram.mid(HeaderSize, PixelCount);

    // QByteArray testPixels = frame.pixels;
    // testPixels[65] =
    //     static_cast<char>(
    //         ThermalQuantization::AboveRangeValue
    //     );

    // const Hotspot hotspot =
    //     HotspotDetector::detect(testPixels);

    // qDebug()
    //     << "Hotspot:"
    //     << "valid =" << hotspot.valid
    //     << "x =" << hotspot.x
    //     << "y =" << hotspot.y
    //     << "temperature =" << hotspot.temperatureCelsius
    //     << "aboveRange =" << hotspot.aboveRange;

    errorMessage.clear();
    return true;

}
