#include <QDebug>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#include "thermal_image_provider.h"

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
     * Compact approximation of Matplotlib's "inferno" map.
     *
     * The original Python receiver used cmap="inferno"; keeping the same
     * dark-purple -> orange -> pale-yellow progression makes low contrast
     * thermal frames much easier to read than the previous rainbow-style HSV
     * ramp, especially around the middle temperatures.
     */
    constexpr std::array<PaletteStop, 9> InfernoStops = {{
        {0.000,   0,   0,   4},
        {0.125,  31,  12,  72},
        {0.250,  85,  15, 109},
        {0.375, 136,  34, 106},
        {0.500, 186,  54,  85},
        {0.625, 227,  89,  51},
        {0.750, 249, 140,  10},
        {0.875, 249, 201,  50},
        {1.000, 252, 255, 164},
    }};

    QRgb sampleInfernoPalette(int thermalIndex)
    {
        const double position =
            static_cast<double>(thermalIndex)
            / 253.0;

        for (qsizetype index = 1;
             index < static_cast<qsizetype>(InfernoStops.size());
             ++index)
        {
            const PaletteStop &right =
                InfernoStops[index];

            if (position > right.position)
                continue;

            const PaletteStop &left =
                InfernoStops[index - 1];

            const double span =
                right.position - left.position;

            const double localPosition =
                span > 0.0
                    ? (position - left.position) / span
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
            sampleInfernoPalette(value - 1)
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
    const FrameStatistics &statistics
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

    // Bicubic interpolation needs a 4 x 4 neighborhood.
    // If that neighborhood touches a sentinel, do not blend it into fake heat.
    if (hasSentinelInBicubicNeighborhood(anchorX, anchorY))
    {
        // Nearest-neighbor keeps invalid/reserved pixels crisp and honest.
        return nearestSourceColor(sourceX, sourceY);
    }

    // Bicubic returns a scalar palette index, not an RGB color.
    const double sampledValue =
        bicubicSample(sourceX, sourceY);

    // Clamp protects against small bicubic overshoots near strong edges.
    const int paletteIndex =
        std::clamp(
            static_cast<int>(std::lround(sampledValue)),
            1,
            254
        );

    // Colorize after interpolation, like imshow samples data then applies cmap.
    return m_colorTable[paletteIndex];
}

double ThermalImageProvider::bicubicSample(
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
            sourcePixel(anchorX - 1, anchorY + rowOffset);

        // p1 is the sample at the anchor in this row.
        const double p1 =
            sourcePixel(anchorX, anchorY + rowOffset);

        // p2 is the sample one pixel after the anchor in this row.
        const double p2 =
            sourcePixel(anchorX + 1, anchorY + rowOffset);

        // p3 is the sample two pixels after the anchor in this row.
        const double p3 =
            sourcePixel(anchorX + 2, anchorY + rowOffset);

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

uchar ThermalImageProvider::sourcePixel(int x, int y) const
{
    // Clamp x so edge pixels can still form a complete 4-sample cubic group.
    const int clampedX =
        std::clamp(x, 0, ImageWidth - 1);

    // Clamp y for the same reason at the top and bottom image borders.
    const int clampedY =
        std::clamp(y, 0, ImageHeight - 1);

    // Read the indexed thermal value from the native 32 x 24 source image.
    return m_image.constScanLine(clampedY)[clampedX];
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
            const uchar value =
                sourcePixel(x, y);

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

    // Read the nearest scalar value, still clamped at image edges.
    const uchar nearestValue =
        sourcePixel(nearestX, nearestY);

    // Convert the nearest scalar value to its existing palette color.
    return m_colorTable[nearestValue];
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
