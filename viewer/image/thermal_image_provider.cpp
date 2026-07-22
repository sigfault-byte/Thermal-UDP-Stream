#include <QDebug>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#include "thermal_image_provider.h"
#include "protocol/thermal_quantization.h"

namespace
{
    struct PaletteStop
    {
        double position;
        int red;
        int green;
        int blue;
    };

    /*
     * Denser approximation of Matplotlib's "inferno" colormap.
     *
     * The original inferno ramp was designed by Nathaniel J. Smith and
     * Stefan van der Walt for the BIDS/matplotlib colormap project and is
     * released as CC0/public-domain data:
     *     https://github.com/BIDS/colormap
     *
     * We keep representative RGB stops here and expand them to a 256-entry
     * table at startup. Smooth mode samples this table after interpolating
     * normalized scalar values, which matches Matplotlib's visual pipeline
     * much more closely than interpolating already-quantized palette indices.
     */
    constexpr std::array<PaletteStop, 18> InfernoStops = {{
        {0.0000,   0,   0,   4},
        {0.0588,   9,   6,  38},
        {0.1176,  27,  12,  65},
        {0.1765,  47,  10,  84},
        {0.2353,  70,  10,  93},
        {0.2941,  94,  16,  91},
        {0.3529, 120,  28,  85},
        {0.4118, 144,  38,  76},
        {0.4706, 169,  47,  66},
        {0.5294, 193,  58,  53},
        {0.5882, 215,  72,  39},
        {0.6471, 234,  91,  23},
        {0.7059, 248, 115,   9},
        {0.7647, 251, 146,   6},
        {0.8235, 248, 181,  28},
        {0.8824, 246, 215,  70},
        {0.9412, 250, 242, 132},
        {1.0000, 252, 255, 164},
    }};

    QRgb sampleInfernoPalette(double position)
    {
        const double clampedPosition =
            std::clamp(position, 0.0, 1.0);

        for (qsizetype index = 1;
             index < static_cast<qsizetype>(InfernoStops.size());
             ++index)
        {
            const PaletteStop &right =
                InfernoStops[index];

            if (clampedPosition > right.position)
                continue;

            const PaletteStop &left =
                InfernoStops[index - 1];

            const double span =
                right.position - left.position;

            const double localPosition =
                span > 0.0
                    ? (clampedPosition - left.position) / span
                    : 0.0;

            const auto interpolate =
                [localPosition](int start, int end)
                {
                    return static_cast<int>(
                        std::lround(
                            start
                            + (end - start) * localPosition
                        )
                    );
                };

            return qRgb(
                interpolate(left.red, right.red),
                interpolate(left.green, right.green),
                interpolate(left.blue, right.blue)
            );
        }

        const PaletteStop &last =
            InfernoStops.back();

        return qRgb(
            last.red,
            last.green,
            last.blue
        );
    }
}

ThermalImageProvider::ThermalImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image),
      m_image(
          ImageWidth,
          ImageHeight,
          QImage::Format_Indexed8
      ),
      m_colorTable(createColorTable())
{
    // indexed images do not contain full RGB values for every pixel.
    // they need a separate table mapping indices 0–255 to RGB colors.
    m_image.setColorTable(m_colorTable);

    // Begin with every pixel using palette index 0.
    m_image.fill(0);
}

QVector<QRgb> ThermalImageProvider::createColorTable()
{
    QVector<QRgb> colorTable;
    colorTable.reserve(256);

    for (int value = 0; value < 256; ++value)
    {
        if (
            value == InvalidValue
            || value == ReservedValue
        )
        {
            colorTable.append(qRgb(0, 0, 0));
            continue;
        }

        // Protocol values 1..254 are the actual thermal ramp.
        colorTable.append(
            sampleInfernoPalette(
                static_cast<double>(value - 1)
                / 253.0
            )
        );
    }

    // Keep sentinel entries distinct from real temperatures.
    colorTable[0] =
        qRgb(255, 0, 255);

    colorTable[255] =
        qRgb(255, 255, 255);

    return colorTable;
}

void ThermalImageProvider::updateFrame(
    const QByteArray &pixels,
    const FrameStatistics &statistics,
    quint8 quantizationMode
)
{
    if (pixels.size() != PixelCount)
    {
        // shoulnt be possible, because udp_receiver checks
        // but this should be very fast should be ok.
        qWarning()
            << "Cannot create thermal image: expected"
            << PixelCount
            << "pixels, received"
            << pixels.size();

        return;
    }

    m_rawPixels = pixels;
    m_statistics = statistics;
    m_quantizationMode = quantizationMode;

    switch (m_scaleMode)
    {
    case FrameModel::ScaleMode::Raw:
        updateRawFrame(pixels);
        break;

    case FrameModel::ScaleMode::Auto:
        updateAutoFrame(
            pixels,
            statistics
        );
        break;
    }
}


QImage ThermalImageProvider::requestImage(
    const QString &id,
    QSize *size,
    const QSize &requestedSize
)
{
    // only one image, so the ID is irrelevant.
    Q_UNUSED(id);

    // QML still owns the final on-screen scaling.
    // The provider only chooses whether QML starts from 32 x 24 or 128 x 96.
    Q_UNUSED(requestedSize);

    if (m_smoothDisplay)
    {
        QImage smoothImage =
            createSmoothImage();

        if (size != nullptr)
        {
            *size = smoothImage.size();
        }

        return smoothImage;
    }

    if (size != nullptr)
    {
        *size = m_image.size();
    }

    return m_image;
}

// scale mode raw/ auto
void ThermalImageProvider::setScaleMode(
    FrameModel::ScaleMode mode
)
{
    m_scaleMode = mode;
}

void ThermalImageProvider::setSmoothDisplay(bool enabled)
{
    m_smoothDisplay = enabled;
}

void ThermalImageProvider::updateRawFrame(
    const QByteArray &pixels
)
{
    for (int row = 0; row < ImageHeight; ++row)
    {
        uchar *destinationRow =
            m_image.scanLine(row);

        for (int column = 0; column < ImageWidth; ++column)
        {
            const int sourceColumn =
                ImageWidth - 1 - column;

            const int sourceIndex =
                row * ImageWidth + sourceColumn;

            destinationRow[column] =
                static_cast<uchar>(
                    pixels[sourceIndex]
                );
        }
    }
}

QImage ThermalImageProvider::createSmoothImage() const
{
    // The smooth image is display-only, so use full RGB pixels after sampling.
    QImage smoothImage(
        SmoothImageWidth,
        SmoothImageHeight,
        QImage::Format_RGB32
    );

    // Visit each output row in the 4x larger display image.
    for (int outputY = 0; outputY < SmoothImageHeight; ++outputY)
    {
        // Ask Qt for the writable memory of this RGB output row.
        QRgb *destinationRow =
            reinterpret_cast<QRgb *>(
                smoothImage.scanLine(outputY)
            );

        // Visit each output column in the 4x larger display image.
        for (int outputX = 0; outputX < SmoothImageWidth; ++outputX)
        {
            // Compute exactly one upscaled display color for this output pixel.
            destinationRow[outputX] =
                smoothPixelColor(outputX, outputY);
        }
    }

    return smoothImage;
}

QRgb ThermalImageProvider::smoothPixelColor(
    int outputX,
    int outputY
) const
{
    // Convert the output pixel center into source-image coordinates.
    // Example: outputX 0 at 4x scale maps near sourceX -0.375.
    // The -0.5 keeps pixel centers aligned rather than aligning pixel edges.
    const double sourceX =
        (static_cast<double>(outputX) + 0.5)
        / SmoothScaleFactor
        - 0.5;

    // Do the same center-aligned mapping for the vertical coordinate.
    const double sourceY =
        (static_cast<double>(outputY) + 0.5)
        / SmoothScaleFactor
        - 0.5;

    // The anchor is the integer source pixel immediately to the left.
    const int anchorX =
        static_cast<int>(
            std::floor(sourceX)
        );

    // The vertical anchor is the integer source pixel immediately above.
    const int anchorY =
        static_cast<int>(
            std::floor(sourceY)
        );

    // Bicubic interpolation needs a 4 x 4 raw-sensor neighborhood.
    // If that neighborhood touches a sentinel, do not blend a label into heat.
    if (hasSentinelInBicubicNeighborhood(anchorX, anchorY))
    {
        // Nearest-neighbor keeps invalid/reserved pixels crisp and honest.
        return nearestSourceColor(sourceX, sourceY);
    }

    // Bicubic returns a normalized scalar: 0.0 is display minimum, 1.0 maximum.
    const double sampledValue =
        bicubicNormalizedSample(sourceX, sourceY);

    // Clamp protects against small bicubic overshoots near strong thermal edges.
    const double normalizedValue =
        std::clamp(
            sampledValue,
            0.0,
            1.0
        );

    // Colorize after scalar interpolation, matching Matplotlib's imshow flow.
    return infernoColor(normalizedValue);
}

double ThermalImageProvider::bicubicNormalizedSample(
    double sourceX,
    double sourceY
) const
{
    // The anchor is the source pixel just before the floating x position.
    const int anchorX =
        static_cast<int>(
            std::floor(sourceX)
        );

    // The anchor is the source pixel just before the floating y position.
    const int anchorY =
        static_cast<int>(
            std::floor(sourceY)
        );

    // fractionX is how far we are between anchorX and anchorX + 1.
    const double fractionX =
        sourceX - anchorX;

    // fractionY is how far we are between anchorY and anchorY + 1.
    const double fractionY =
        sourceY - anchorY;

    // Each row result stores one horizontal cubic interpolation.
    double rowSamples[4] = {};

    // Bicubic first performs cubic interpolation across four rows.
    for (int rowOffset = -1; rowOffset <= 2; ++rowOffset)
    {
        // p0 is the sample one pixel before the anchor in this row.
        const double p0 =
            normalizedSourceValue(anchorX - 1, anchorY + rowOffset);

        // p1 is the normalized sample at the anchor in this row.
        const double p1 =
            normalizedSourceValue(anchorX, anchorY + rowOffset);

        // p2 is the normalized sample one pixel after the anchor in this row.
        const double p2 =
            normalizedSourceValue(anchorX + 1, anchorY + rowOffset);

        // p3 is the normalized sample two pixels after the anchor in this row.
        const double p3 =
            normalizedSourceValue(anchorX + 2, anchorY + rowOffset);

        // Store this row's interpolated value at the fractional x position.
        rowSamples[rowOffset + 1] =
            cubicInterpolate(
                p0,
                p1,
                p2,
                p3,
                fractionX
            );
    }

    // Then interpolate vertically through the four row results.
    return cubicInterpolate(
        rowSamples[0],
        rowSamples[1],
        rowSamples[2],
        rowSamples[3],
        fractionY
    );
}

double ThermalImageProvider::cubicInterpolate(
    double p0,
    double p1,
    double p2,
    double p3,
    double fraction
) const
{
    // This is the Catmull-Rom cubic used here as the bicubic building block.
    // It passes smoothly between p1 and p2 while using p0 and p3 for slope.
    return p1
        + 0.5
        * fraction
        * (
            p2 - p0
            + fraction
            * (
                (2.0 * p0)
                - (5.0 * p1)
                + (4.0 * p2)
                - p3
                + fraction
                * (
                    (3.0 * (p1 - p2))
                    + p3
                    - p0
                )
            )
        );
}

quint8 ThermalImageProvider::rawSourcePixel(int x, int y) const
{
    // Clamp display-space x so border pixels can form a complete 4x4 group.
    const int clampedX =
        std::clamp(x, 0, ImageWidth - 1);

    // Clamp display-space y for the top and bottom image borders.
    const int clampedY =
        std::clamp(y, 0, ImageHeight - 1);

    // The old display path flips horizontally while copying into m_image.
    // Smooth reads the raw payload, so reproduce that same display orientation.
    const int rawX =
        ImageWidth - 1 - clampedX;

    // Convert the two-dimensional sensor coordinate back to row-major offset.
    const int rawIndex =
        clampedY * ImageWidth + rawX;

    // If no frame has arrived yet, show the below-range sentinel color.
    if (m_rawPixels.size() != PixelCount)
    {
        return InvalidValue;
    }

    // Return the original byte from the sensor payload, before display mapping.
    return static_cast<quint8>(
        m_rawPixels[rawIndex]
    );
}

double ThermalImageProvider::normalizedSourceValue(int x, int y) const
{
    // Read the raw thermal byte at this display-space source coordinate.
    const quint8 rawValue =
        rawSourcePixel(x, y);

    // This function should only be called after sentinel-neighborhood checks.
    // Keep the guard anyway so accidental sentinel reads become harmless edges.
    if (
        rawValue == InvalidValue
        || rawValue == ReservedValue
    )
    {
        return rawValue == ReservedValue
            ? 1.0
            : 0.0;
    }

    // Convert encoded sensor bytes to Celsius before normalizing the display.
    const double temperatureCelsius =
        ThermalQuantization::decodeTemperature(
            rawValue,
            m_quantizationMode
        );

    double displayMinimumCelsius =
        ThermalQuantization::rangeForMode(
            m_quantizationMode
        ).minimumTemperatureC;

    double displayMaximumCelsius =
        ThermalQuantization::rangeForMode(
            m_quantizationMode
        ).maximumTemperatureC;

    // Auto mode uses the current frame's real in-range min/max as vmin/vmax.
    if (
        m_scaleMode == FrameModel::ScaleMode::Auto
        && m_statistics.inRangePixelCount > 0
    )
    {
        displayMinimumCelsius =
            m_statistics.minimumCelsius;

        displayMaximumCelsius =
            m_statistics.maximumCelsius;
    }

    // A flat frame has no contrast range, so place valid pixels in the middle.
    if (qFuzzyCompare(displayMinimumCelsius, displayMaximumCelsius))
    {
        return 0.5;
    }

    // Matplotlib-style normalization: (value - vmin) / (vmax - vmin).
    const double normalizedValue =
        (
            temperatureCelsius
            - displayMinimumCelsius
        )
        / (
            displayMaximumCelsius
            - displayMinimumCelsius
        );

    // Clamp because raw mode can see values outside its visible display range.
    return std::clamp(
        normalizedValue,
        0.0,
        1.0
    );
}

QRgb ThermalImageProvider::infernoColor(
    double normalizedValue
) const
{
    // Convert normalized 0.0..1.0 scalar position into a 256-entry LUT index.
    const int colorIndex =
        std::clamp(
            static_cast<int>(
                std::lround(
                    std::clamp(normalizedValue, 0.0, 1.0)
                    * 253.0
                )
            )
            + 1,
            1,
            254
        );

    // The table reserves 0 and 255 for sentinel colors, so normal data uses 1..254.
    return m_colorTable[colorIndex];
}

bool ThermalImageProvider::hasSentinelInBicubicNeighborhood(
    int anchorX,
    int anchorY
) const
{
    // Bicubic needs x samples anchor - 1, anchor, anchor + 1, anchor + 2.
    for (int y = anchorY - 1; y <= anchorY + 2; ++y)
    {
        // For each of those four rows, inspect the four needed columns.
        for (int x = anchorX - 1; x <= anchorX + 2; ++x)
        {
            // Read with clamping so border neighborhoods are valid.
            const quint8 value =
                rawSourcePixel(x, y);

            // Invalid and reserved values are labels, not temperatures.
            if (
                value == InvalidValue
                || value == ReservedValue
            )
            {
                // Returning true tells the caller not to smear this label.
                return true;
            }
        }
    }

    return false;
}

QRgb ThermalImageProvider::nearestSourceColor(
    double sourceX,
    double sourceY
) const
{
    // Round to the source pixel whose center is closest to the output pixel.
    const int nearestX =
        static_cast<int>(
            std::lround(sourceX)
        );

    // Round vertically in the same nearest-neighbor way.
    const int nearestY =
        static_cast<int>(
            std::lround(sourceY)
        );

    // Read the nearest raw label/value, still clamped at image edges.
    const quint8 nearestValue =
        rawSourcePixel(nearestX, nearestY);

    // Preserve below/above-range labels as their distinct display colors.
    if (
        nearestValue == InvalidValue
        || nearestValue == ReservedValue
    )
    {
        return m_colorTable[nearestValue];
    }

    // For normal thermal values, normalize Celsius and then colorize.
    const double normalizedValue =
        normalizedSourceValue(nearestX, nearestY);

    // Return the inferno color for the nearest real thermal sample.
    return infernoColor(normalizedValue);
}

void ThermalImageProvider::updateAutoFrame(
    const QByteArray &pixels,
    const FrameStatistics &statistics
)
{
    /*
     * No in-range pixel exists.
     * Preserve the sentinel values as they are.
     */
    if (statistics.inRangePixelCount == 0)
    {
        updateRawFrame(pixels);
        return;
    }

    // Auto scaling works in the same encoded 1..254 domain
    // as the incoming pixel values.
    const quint8 minimum =
        statistics.minimumEncoded;

    const quint8 maximum =
        statistics.maximumEncoded;

    const bool flatFrame =
        minimum == maximum;

    for (int row = 0; row < ImageHeight; ++row)
    {
        uchar *destinationRow =
            m_image.scanLine(row);

        for (int column = 0; column < ImageWidth; ++column)
        {
            const int sourceColumn =
                ImageWidth - 1 - column;

            const int sourceIndex =
                row * ImageWidth + sourceColumn;

            const quint8 value =
                static_cast<quint8>(
                    pixels[sourceIndex]
                );

            if (
                value == InvalidValue
                || value == ReservedValue
            )
            {
                destinationRow[column] = value;
                continue;
            }

            if (flatFrame)
            {
                destinationRow[column] = 127;
                continue;
            }

            const int numerator =
                (value - minimum) * 253;

            const int denominator =
                maximum - minimum;

            const int mappedValue =
                1 + (numerator / denominator);

            destinationRow[column] =
                static_cast<uchar>(mappedValue);
        }
    }
}
