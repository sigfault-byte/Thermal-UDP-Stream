#include <QDebug>

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

    // deliberately return the native 32 × 24 image.
    // QML will enlarge it while preserving the sharp pixel blocks.
    Q_UNUSED(requestedSize);

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
