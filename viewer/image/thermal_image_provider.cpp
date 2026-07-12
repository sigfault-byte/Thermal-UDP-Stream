#include <QColor>
#include <QDebug>

#include <cstring>

#include "thermal_image_provider.h"

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
        /*
         * convert the byte value into an HSV color.
         *
         * value 0:
         *     hue approximately 240° → blue
         *     low brightness
         *
         * value 255:
         *     hue approximately 0° → red
         *     full brightness
         *
         * Every byte therefore selects one color from a
         * 256-entry blue-to-red thermal palette.
         */

        const int hue =
            240 - ((value * 240) / 255);

        const int brightness =
            48 + ((value * 207) / 255);

        const QColor color =
            QColor::fromHsv(
                hue,
                255,
                brightness
            );

        // Store the RGB representation in the color table.
        colorTable.append(color.rgb());


    }
    // override the sentinel entries
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
