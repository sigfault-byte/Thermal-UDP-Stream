#include "thermal_image_provider.h"

#include <QColor>
#include <QDebug>

#include <cstring>

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

    return colorTable;
}

void ThermalImageProvider::updateFrame(
    const QByteArray &pixels
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

    /*
     * copy the payload one row at a time.
     *
     * do not assume that the memory occupied by one QImage row
     * is always exactly equal to ImageWidthSome
     * image formats may add padding at the end of each row.... !
     */
    for (int row = 0; row < ImageHeight; ++row)
    {
        uchar *destinationRow =
            m_image.scanLine(row);

        const char *sourceRow =
            pixels.constData() + (row * ImageWidth);

        std::memcpy(
            destinationRow,
            sourceRow,
            ImageWidth
        );
    }
}

QImage ThermalImageProvider::requestImage(
    const QString &id,
    QSize *size,
    const QSize &requestedSize
)
{
    // We currently have only one image, so the ID is irrelevant.
    Q_UNUSED(id);

    // We deliberately return the native 32 × 24 image.
    // QML will enlarge it while preserving the sharp pixel blocks.
    Q_UNUSED(requestedSize);

    if (size != nullptr)
    {
        *size = m_image.size();
    }

    return m_image;
}
