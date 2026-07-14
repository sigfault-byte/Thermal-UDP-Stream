#ifndef THERMAL_QUANTIZATION_H
#define THERMAL_QUANTIZATION_H

#include <QtGlobal>
#include <QString>

#include <cmath>
#include <algorithm>

namespace ThermalQuantization
{

struct Range
{
    quint8 mode = 1;
    double minimumTemperatureC = 10.0;
    double maximumTemperatureC = 45.0;
};

constexpr quint8 ModeDefault = 1;
constexpr quint8 ModeWarm = 2;
constexpr quint8 ModeWide = 3;

constexpr quint8 BelowRangeValue = 0;
constexpr quint8 MinimumEncodedValue = 1;
constexpr quint8 MaximumEncodedValue = 254;
constexpr quint8 AboveRangeValue = 255;

constexpr int EncodedIntervalCount =
    MaximumEncodedValue - MinimumEncodedValue;

inline bool isValidMode(quint8 mode)
{
    return mode == ModeDefault
        || mode == ModeWarm
        || mode == ModeWide;
}

inline Range rangeForMode(quint8 mode)
{
    if (mode == ModeWarm)
    {
        return Range{mode, 20.0, 60.0};
    }

    if (mode == ModeWide)
    {
        return Range{mode, 0.0, 100.0};
    }

    // Mode 1 is the default and preserves the old 10-45 C behavior.
    return Range{ModeDefault, 10.0, 45.0};
}

inline double rangeTemperatureC(quint8 mode)
{
    const Range range =
        rangeForMode(mode);

    return range.maximumTemperatureC
        - range.minimumTemperatureC;
}

inline double celsiusPerEncodedStep(quint8 mode)
{
    return rangeTemperatureC(mode)
        / static_cast<double>(EncodedIntervalCount);
}

inline double encodedStepsPerCelsius(quint8 mode)
{
    return static_cast<double>(EncodedIntervalCount)
        / rangeTemperatureC(mode);
}

inline bool isValidTemperatureValue(quint8 value)
{
    return value >= MinimumEncodedValue
        && value <= MaximumEncodedValue;
}

inline double decodeTemperature(
    double encodedValue,
    quint8 mode
)
{
    const double encodedPosition =
        encodedValue - MinimumEncodedValue;

    return rangeForMode(mode).minimumTemperatureC
        + encodedPosition * celsiusPerEncodedStep(mode);
}

inline int encodeTemperatureDifference(
    double differenceCelsius,
    quint8 mode
)
{
    return static_cast<int>(
        std::lround(
            differenceCelsius
            * encodedStepsPerCelsius(mode)
        )
    );
}

inline quint8 encodeTemperature(
    double temperatureCelsius,
    quint8 mode
)
{
    const Range range =
        rangeForMode(mode);

    if (temperatureCelsius < range.minimumTemperatureC)
    {
        return BelowRangeValue;
    }

    if (temperatureCelsius > range.maximumTemperatureC)
    {
        return AboveRangeValue;
    }

    const long encoded =
        std::lround(
            static_cast<double>(MinimumEncodedValue)
            + (
                temperatureCelsius
                - range.minimumTemperatureC
            ) * encodedStepsPerCelsius(mode)
        );

    return static_cast<quint8>(encoded);
}

inline QString rangeText(quint8 mode)
{
    const Range range =
        rangeForMode(mode);

    return QString("%1-%2 C")
        .arg(range.minimumTemperatureC, 0, 'f', 0)
        .arg(range.maximumTemperatureC, 0, 'f', 0);
}

}

#endif
