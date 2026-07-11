#ifndef THERMAL_IMAGE_PROVIDER_H
#define THERMAL_IMAGE_PROVIDER_H

#include <QByteArray>
#include <QImage>
#include <QQuickImageProvider>
#include <QVector>

#include "models/frame_model.h"
#include "analysis/frame_statistics.h"

class ThermalImageProvider : public QQuickImageProvider
{
public:
    ThermalImageProvider();

    // Select how incoming thermal values are mapped to palette indices.
    void setScaleMode(FrameModel::ScaleMode mode);

    // replace the currently displayed image using one exact
    // 32 × 24 thermal payload :d
    void updateFrame(
        const QByteArray &pixels,
        const FrameStatistics &statistics
    );

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

    static constexpr quint8 InvalidValue = 0;
    static constexpr quint8 ReservedValue = 255;

    // Build the 256-entry lookup table.
    static QVector<QRgb> createColorTable();

    void updateRawFrame(const QByteArray &pixels);
    void updateAutoFrame(
        const QByteArray &pixels,
        const FrameStatistics &statistics
    );

    // The most recently received thermal frame represented
    // as an indexed-color image.
    QImage m_image;

    // 256 RGB colors. Each incoming byte selects one entry.
    QVector<QRgb> m_colorTable;

    FrameModel::ScaleMode m_scaleMode =
        FrameModel::ScaleMode::Raw;
};

#endif
