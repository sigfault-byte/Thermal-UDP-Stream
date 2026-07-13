#include "hotspot_detector.h"
#include "thermal_frame_constants.h"
#include "protocol/thermal_quantization.h"

#include <algorithm>
#include <limits>
#include <QDebug>

namespace
{
constexpr int TemperatureToleranceEncoded = 7;
constexpr int MaxRadius = 12;
constexpr double ColdPixelPenalty = 1.0;
}

Hotspot HotspotDetector::detect(const QByteArray &pixels)
{
    Hotspot hotspot;
    quint8 hottestEncoded = 0;

     // First pass: locate the hottest pixel
     // Parse a flat row major array representing a 32 × 24 frame.
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

            hotspot.peakX =
                static_cast<int>(
                    index % ThermalFrameConstants::Width
                );

            hotspot.peakY =
                static_cast<int>(
                    index / ThermalFrameConstants::Width
                );

            hotspot.peakAboveRange =
                value == ThermalQuantization::AboveRangeValue;

            if (hotspot.peakAboveRange)
            {
                /*
                 * The exact temperature is unknown.
                 * This stores the known lower boundary.
                 */
                hotspot.peakTemperatureCelsius = 45.0;
            }
            else
            {
                hotspot.peakTemperatureCelsius =
                    ThermalQuantization::decodeTemperature(value);
            }
        }
    }

    /*
     * The frame contained only below-range pixels.
     */
    if (!hotspot.valid)
    {
        return hotspot;
    }

    /*
     * Pixels no more than seven encoded steps below
     * the peak are currently classified as hot.
     */
    const int hotThresholdEncoded =
        std::max(
            1,
            static_cast<int>(hottestEncoded)
                - TemperatureToleranceEncoded
        );

    /*
     * Remember the best circle encountered so far.
     *
     * lowest() is used because candidate scores
     * are allowed to be negative.
     */
    double bestScore =
        std::numeric_limits<double>::lowest();

    int bestCenterX = hotspot.peakX;
    int bestCenterY = hotspot.peakY;
    int bestRadius = 1;
    int bestHotPixelCount = 1;
    int bestTotalPixelCount = 1;

    /*
     * Try every sensor pixel as a possible circle center.
     */
    for (
        int centerY = 0;
        centerY < ThermalFrameConstants::Height;
        ++centerY
    )
    {
        for (
            int centerX = 0;
            centerX < ThermalFrameConstants::Width;
            ++centerX
        )
        {
            /*
             * Try every permitted radius for this center.
             */
            for (
                int radius = 1;
                radius <= MaxRadius;
                ++radius
            )
            {
                int hotPixelCount = 0;
                int totalPixelCount = 0;

                const int peakDeltaX =
                    hotspot.peakX - centerX;

                const int peakDeltaY =
                    hotspot.peakY - centerY;

                const int peakDistanceSquared =
                    peakDeltaX * peakDeltaX
                    + peakDeltaY * peakDeltaY;

                const int radiusSquared =
                    radius * radius;

                if (peakDistanceSquared > radiusSquared)
                {
                    continue;
                }

                /*
                 * Inspect every frame pixel and determine
                 * whether its center lies inside this circle.
                 */
                for (
                    qsizetype index = 0;
                    index < pixels.size();
                    ++index
                )
                {
                    const int pixelX =
                        static_cast<int>(
                            index
                            % ThermalFrameConstants::Width
                        );

                    const int pixelY =
                        static_cast<int>(
                            index
                            / ThermalFrameConstants::Width
                        );

                    const int deltaX =
                        pixelX - centerX;

                    const int deltaY =
                        pixelY - centerY;

                    const int distanceSquared =
                        deltaX * deltaX
                        + deltaY * deltaY;

                    if (distanceSquared > radiusSquared)
                    {
                        continue;
                    }

                    ++totalPixelCount;

                    const quint8 value =
                        static_cast<quint8>(
                            pixels[index]
                        );

                    const bool isHot =
                        value
                            != ThermalQuantization::BelowRangeValue
                        && static_cast<int>(value)
                            >= hotThresholdEncoded;

                    if (isHot)
                    {
                        ++hotPixelCount;
                    }
                }

                const int coldPixelCount =
                    totalPixelCount - hotPixelCount;

                const double score =
                    static_cast<double>(hotPixelCount)
                    - ColdPixelPenalty
                        * static_cast<double>(
                            coldPixelCount
                        );

                if (score > bestScore)
                {
                    bestScore = score;
                    bestCenterX = centerX;
                    bestCenterY = centerY;
                    bestRadius = radius;
                    bestHotPixelCount = hotPixelCount;
                    bestTotalPixelCount = totalPixelCount;
                }
            }
        }
    }

    /*
     * Store the winning candidate.
     */
    hotspot.centerX =
        static_cast<double>(bestCenterX);

    hotspot.centerY =
        static_cast<double>(bestCenterY);

    hotspot.radiusPixels =
        static_cast<double>(bestRadius);

    hotspot.hotPixelCount =
        bestHotPixelCount;

    hotspot.totalPixelCount =
        bestTotalPixelCount;

    hotspot.score =
        bestScore;

    qDebug()
        << "Peak:"
        << hotspot.peakX
        << hotspot.peakY
        << hotspot.peakTemperatureCelsius
        << "Circle:"
        << hotspot.centerX
        << hotspot.centerY
        << hotspot.radiusPixels
        << "hot:"
        << hotspot.hotPixelCount
        << "total:"
        << hotspot.totalPixelCount
        << "score:"
        << hotspot.score;

    return hotspot;
}
