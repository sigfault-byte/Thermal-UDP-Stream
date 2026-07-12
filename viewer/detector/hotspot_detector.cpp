#include "hotspot_detector.h"
#include "thermal_frame_constants.h"
#include "protocol/thermal_quantization.h"

Hotspot HotspotDetector::detect(const QByteArray &pixels)
{
    Hotspot hotspot;

    quint8 hottestEncoded = 0;

    // Parse a flat row-major array representing a 32 × 24 frame.
    // convention will be top left x,y = (0;0)
    // if index = 12
    // x = 12, y = 0
    // if index = 54
    // x = 54 % 32 = 22 y = 54 / 32 = 1

    for (qsizetype index = 0; index < pixels.size(); ++index)
    {
        const quint8 value =
            static_cast<quint8>(pixels[index]);
        if (value == ThermalQuantization::BelowRangeValue)
        {
            continue;
        }
        if (!hotspot.valid || value > hottestEncoded)
        {
            hottestEncoded = value;
            hotspot.valid = true;
            hotspot.x =
                static_cast<int>(
                    index % ThermalFrameConstants::Width
                );
            hotspot.y =
                static_cast<int>(
                    index / ThermalFrameConstants::Width
                );
            hotspot.aboveRange =
                value == ThermalQuantization::AboveRangeValue;
            if (hotspot.aboveRange)
            {
                hotspot.temperatureCelsius = 0.0;
            }
            else
            {
                hotspot.temperatureCelsius =
                    ThermalQuantization::decodeTemperature(value);
            }
        }
    }

    return hotspot;
}
