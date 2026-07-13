#include "hotspot_detector.h"
#include "hotspot.h"
#include "hotspot_settings.h"
#include "thermal_frame_constants.h"
#include "protocol/thermal_quantization.h"

#include <algorithm>
#include <limits>
// #include <QDebug>

/*
namespace
{

 * Encoded pixels are bytes, not Celsius values.
 *
 * A pixel is considered part of the hot region when it is close enough
 * to the hottest pixel in the frame.

constexpr int TemperatureToleranceEncoded = 7;


 * The detector tries candidate circles from radius 1 through this value.
 * Radius is measured in sensor pixels, not screen pixels.

constexpr int MaxRadius = 12;

 * Candidate score is:
 *
 *     hot pixels - ColdPixelPenalty * cold pixels
 *
 * With a penalty of 1.0 this is simply hot - cold.

constexpr double ColdPixelPenalty = 1.0;
}
*/

Hotspot HotspotDetector::detect(const QByteArray &pixels, const HotspotSettings &settings)
{
    Hotspot hotspot;
    quint8 hottestEncoded = 0;

    const int temperatureToleranceEncoded =
        settings.temperatureToleranceEncoded();

     // First pass: locate the hottest pixel
     // Parse a flat row major array representing a 32 × 24 frame.
     // convention will be top left x,y = (0;0)
     // if index = 12
     // x = 12, y = 0
     // if index = 54
     // x = 54 % 32 = 22 y = 54 / 32 = 1
     //
     // The frame is a flat buffer, but conceptually it is:
     //
     //   (0,0)                          (31,0)
     //     +------------------------------+
     //     | 0   1   2   ...          31  |
     //     | 32  33  34  ...          63  |
     //     | ...                          |
     //     | 736 ...                  767 |
     //     +------------------------------+
     //   (0,23)                         (31,23)
    for (qsizetype index = 0; index < pixels.size(); ++index)
    {
        const quint8 value =
            static_cast<quint8>(pixels[index]);

        /*
         * Ignore pixels below the measurable range. They cannot be
         * selected as the peak and are not part of a hot region.
         */
        if (value == ThermalQuantization::BelowRangeValue)
        {
            continue;
        }

        /*
         * Keep the hottest encoded value seen so far and remember
         * its raw sensor coordinate.
         */
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
     *
     * Example:
     *
     *     peak encoded value = 180
     *     threshold          = 173
     *
     * Any in-frame pixel with encoded value >= 173 is counted
     * as hot for the candidate-circle scoring below.
     */
    const int hotThresholdEncoded =
        std::max(
            1,
            static_cast<int>(hottestEncoded)
                - temperatureToleranceEncoded
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
     *
     * Brute force shape:
     *
     *     for each centerY
     *       for each centerX
     *         for each radius
     *           count hot and cold pixels inside that circle
     *
     * Every candidate circle must contain the peak pixel. This keeps
     * the selected region anchored to the hottest point in the frame.
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
                radius <= settings.maximumRadiusPixels;
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

                /*
                 * Reject candidate circles that do not include the
                 * hottest pixel.
                 *
                 *   candidate includes peak:
                 *
                 *        .....
                 *     ..       ..
                 *    .    P      .
                 *   .      C      .
                 *    .           .
                 *     ..       ..
                 *        .....
                 *
                 *   P = peak pixel
                 *   C = candidate center
                 */
                if (peakDistanceSquared > radiusSquared)
                {
                    continue;
                }

                /*
                 * Walk the square bounding box around the circle:
                 *
                 *     +---------+
                 *     |   ***   |
                 *     |  *****  |
                 *     |  **C**  |
                 *     |  *****  |
                 *     |   ***   |
                 *     +---------+
                 *
                 * The distance check below keeps only the positions
                 * marked with '*', which are inside the circle.
                 */
                for (int pixelY = centerY - radius;
                     pixelY <= centerY + radius;
                     ++pixelY)
                {
                    for (int pixelX = centerX - radius;
                         pixelX <= centerX + radius;
                         ++pixelX)
                    {
                        const int deltaX = pixelX - centerX;
                        const int deltaY = pixelY - centerY;

                        if (
                            deltaX * deltaX + deltaY * deltaY
                            > radiusSquared
                        )
                        {
                            continue;
                        }

                        /*
                         * Count every mathematical circle position,
                         * even if it falls outside the sensor frame.
                         *
                         * This intentionally penalizes circles that hang
                         * over the edge, so the chosen hotspot circle
                         * stays mostly within the 32 x 24 sensor area.
                         */
                        ++totalPixelCount;

                        const bool outsideFrame =
                            pixelX < 0
                            || pixelX >= ThermalFrameConstants::Width
                            || pixelY < 0
                            || pixelY >= ThermalFrameConstants::Height;

                        if (outsideFrame)
                        {
                            continue;
                        }

                        const qsizetype index =
                            pixelY * ThermalFrameConstants::Width
                            + pixelX;

                        const quint8 value =
                            static_cast<quint8>(pixels[index]);

                        const bool isHot =
                            value != ThermalQuantization::BelowRangeValue
                            && static_cast<int>(value) >= hotThresholdEncoded;

                        /*
                         * Only real in-frame pixels can increase the hot
                         * count. Out-of-frame positions only contributed
                         * to totalPixelCount above.
                         */
                        if (isHot)
                        {
                            ++hotPixelCount;
                        }
                    }
                }

                const int coldPixelCount =
                    totalPixelCount - hotPixelCount;

                /*
                 * Prefer circles full of hot pixels and reject circles
                 * that include too much cold area.
                 *
                 * With e.g ColdPixelPenalty == 1.0:
                 *
                 *     10 hot,  2 cold  -> score   8
                 *     10 hot,  9 cold  -> score   1
                 *     10 hot, 20 cold  -> score -10
                 */
                const double score =
                    static_cast<double>(hotPixelCount)
                    - settings.coldPixelPenalty
                        * static_cast<double>(coldPixelCount);

                /*
                 * Keep the best-scoring candidate seen so far.
                 * Ties keep the earlier candidate because this only
                 * updates on strictly greater scores.
                 */
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

    double temperatureSumCelsius = 0.0;
    int temperatureSampleCount = 0;
    const int bestRadiusSquared = bestRadius * bestRadius;

    for (int pixelY = bestCenterY - bestRadius;
         pixelY <= bestCenterY + bestRadius;
         ++pixelY)
    {
        for (int pixelX = bestCenterX - bestRadius;
             pixelX <= bestCenterX + bestRadius;
             ++pixelX)
        {
            const int deltaX = pixelX - bestCenterX;
            const int deltaY = pixelY - bestCenterY;

            if (deltaX * deltaX + deltaY * deltaY > bestRadiusSquared)
            {
                continue;
            }

            const bool outsideFrame =
                pixelX < 0
                || pixelX >= ThermalFrameConstants::Width
                || pixelY < 0
                || pixelY >= ThermalFrameConstants::Height;

            if (outsideFrame)
            {
                continue;
            }

            const qsizetype index =
                pixelY * ThermalFrameConstants::Width
                + pixelX;

            const quint8 value =
                static_cast<quint8>(pixels[index]);

            if (!ThermalQuantization::isValidTemperatureValue(value))
            {
                continue;
            }

            temperatureSumCelsius +=
                ThermalQuantization::decodeTemperature(value);
            ++temperatureSampleCount;
        }
    }

    if (temperatureSampleCount > 0)
    {
        hotspot.meanTemperatureCelsius =
            temperatureSumCelsius
            / static_cast<double>(temperatureSampleCount);
    }

    // qDebug()
    //     << "Peak:"
    //     << hotspot.peakX
    //     << hotspot.peakY
    //     << hotspot.peakTemperatureCelsius
    //     << "Circle:"
    //     << hotspot.centerX
    //     << hotspot.centerY
    //     << hotspot.radiusPixels
    //     << "hot:"
    //     << hotspot.hotPixelCount
    //     << "total:"
    //     << hotspot.totalPixelCount
    //     << "score:"
    //     << hotspot.score;

    return hotspot;
}
