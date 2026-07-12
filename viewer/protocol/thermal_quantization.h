#ifndef THERMAL_QUANTIZATION_H
#define THERMAL_QUANTIZATION_H

#include <QtGlobal>

namespace ThermalQuantization
{
    constexpr double MinimumTemperatureC = 10.0;
    constexpr double MaximumTemperatureC = 45.0;

    constexpr quint8 BelowRangeValue = 0;
    constexpr quint8 MinimumEncodedValue = 1;
    constexpr quint8 MaximumEncodedValue = 254;
    constexpr quint8 AboveRangeValue = 255;

    inline bool isValidTemperatureValue(quint8 value)
    {
        return value >= MinimumEncodedValue
            && value <= MaximumEncodedValue;
    }

    inline double decodeTemperature(double encodedValue)
    {
        const double encodedPosition =
            encodedValue - MinimumEncodedValue;

        const double encodedRange =
            MaximumEncodedValue - MinimumEncodedValue;

        const double temperatureRange =
            MaximumTemperatureC - MinimumTemperatureC;

        return MinimumTemperatureC
            + (encodedPosition / encodedRange)
            * temperatureRange;
    }
}

#endif
