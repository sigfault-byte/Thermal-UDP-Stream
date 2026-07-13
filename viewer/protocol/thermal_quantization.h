#ifndef THERMAL_QUANTIZATION_H
#define THERMAL_QUANTIZATION_H

#include <QtGlobal>

#include <cmath>
#include <algorithm>

namespace ThermalQuantization
{

constexpr double MinimumTemperatureC = 10.0;
constexpr double MaximumTemperatureC = 45.0;

constexpr double RangeTemperatureC =
    MaximumTemperatureC - MinimumTemperatureC;

constexpr quint8 BelowRangeValue = 0;
constexpr quint8 MinimumEncodedValue = 1;
constexpr quint8 MaximumEncodedValue = 254;
constexpr quint8 AboveRangeValue = 255;

constexpr int EncodedIntervalCount =
    MaximumEncodedValue - MinimumEncodedValue;

constexpr double CelsiusPerEncodedStep =
    RangeTemperatureC
    / static_cast<double>(EncodedIntervalCount);

constexpr double EncodedStepsPerCelsius =
    static_cast<double>(EncodedIntervalCount)
    / RangeTemperatureC;

inline bool isValidTemperatureValue(quint8 value)
{
    return value >= MinimumEncodedValue
        && value <= MaximumEncodedValue;
}

inline double decodeTemperature(double encodedValue)
{
    const double encodedPosition =
        encodedValue - MinimumEncodedValue;

    return MinimumTemperatureC
        + encodedPosition * CelsiusPerEncodedStep;
}

inline int encodeTemperatureDifference(
    double differenceCelsius
)
{
    return static_cast<int>(
        std::lround(
            differenceCelsius
            * EncodedStepsPerCelsius
        )
    );
}

inline quint8 encodeTemperature(
    double temperatureCelsius
)
{
    if (temperatureCelsius < MinimumTemperatureC)
    {
        return BelowRangeValue;
    }

    if (temperatureCelsius > MaximumTemperatureC)
    {
        return AboveRangeValue;
    }

    const long encoded =
        std::lround(
            static_cast<double>(MinimumEncodedValue)
            + (
                temperatureCelsius
                - MinimumTemperatureC
            ) * EncodedStepsPerCelsius
        );

    return static_cast<quint8>(encoded);
}

}

#endif
