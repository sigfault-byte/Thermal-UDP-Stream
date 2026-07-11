#include "frame_statistics_calculator.h"
#include "frame_statistics.h"
#include "protocol/thermal_quantization.h"

namespace
{
constexpr quint8 InvalidValue = 0;
constexpr quint8 ReservedValue = 255;
}

FrameStatistics FrameStatisticsCalculator::calculate(
    const QByteArray &pixels
)
{
    FrameStatistics statistics;

    quint64 sum = 0;

    quint8 minimum = 254;
    quint8 maximum = 1;

    for (qsizetype index = 0; index < pixels.size(); ++index)
    {
        const quint8 value =
            static_cast<quint8>(pixels[index]);

        if (value == ThermalQuantization::BelowRangeValue)
        {
            ++statistics.belowRangePixelCount;
            continue;
        }

        if (value == ThermalQuantization::AboveRangeValue)
        {
            ++statistics.aboveRangePixelCount;
            continue;
        }

        if (value < minimum)
            minimum = value;

        if (value > maximum)
            maximum = value;

        sum += value;
        ++statistics.inRangePixelCount;
    }

    /*
     * Avoid division by zero when the frame contains
     * only below-range or above-range sentinel values.
     */
    if (statistics.inRangePixelCount == 0)
    {
        return statistics;
    }

    statistics.minimumEncoded = minimum;
    statistics.maximumEncoded = maximum;

    statistics.minimumCelsius =
        ThermalQuantization::decodeTemperature(minimum);

    statistics.maximumCelsius =
        ThermalQuantization::decodeTemperature(maximum);

    const double meanEncoded =
        static_cast<double>(sum)
        / statistics.inRangePixelCount;

    statistics.meanCelsius =
        ThermalQuantization::decodeTemperature(meanEncoded);

    return statistics;
}
