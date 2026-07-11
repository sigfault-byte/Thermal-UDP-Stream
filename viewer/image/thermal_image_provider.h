#ifndef THERMAL_IMAGE_PROVIDER_H
#define THERMAL_IMAGE_PROVIDER_H

#include <QByteArray>
#include <QImage>
#include <QQuickImageProvider>
#include <QVector>

class ThermalImageProvider : public QQuickImageProvider
{
public:
    ThermalImageProvider();

    // replace the currently displayed image using one exact
    // 32 × 24 thermal payload :d
    void updateFrame(const QByteArray &pixels);

    // called by the QML engine when an Image requests:
    //
    //     image://thermal/...
    //
    // the returned QImage becomes the source displayed by QML :cool:
    QImage requestImage(
        const QString &id,
        QSize *size,
        const QSize &requestedSize
    ) override;

private:
    static constexpr int ImageWidth = 32;
    static constexpr int ImageHeight = 24;
    static constexpr int PixelCount = ImageWidth * ImageHeight;

    // Build the 256-entry lookup table.
    static QVector<QRgb> createColorTable();

    // The most recently received thermal frame represented
    // as an indexed-color image.
    QImage m_image;

    // 256 RGB colors. Each incoming byte selects one entry.
    QVector<QRgb> m_colorTable;
};

#endif
