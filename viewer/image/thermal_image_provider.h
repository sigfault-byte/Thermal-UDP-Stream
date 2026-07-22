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

    // Select whether QML receives the native image or a 4x bicubic display image.
    void setSmoothDisplay(bool enabled);

    // replace the currently displayed image using one exact
    // 32 × 24 thermal payload :d
    void updateFrame(
        const QByteArray &pixels,
        const FrameStatistics &statistics,
        quint8 quantizationMode
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
    static constexpr int SmoothScaleFactor = 4;
    static constexpr int SmoothImageWidth = ImageWidth * SmoothScaleFactor;
    static constexpr int SmoothImageHeight = ImageHeight * SmoothScaleFactor;

    static constexpr quint8 InvalidValue = 0;
    static constexpr quint8 ReservedValue = 255;

    // Build the 256-entry lookup table.
    static QVector<QRgb> createColorTable();

    void updateRawFrame(const QByteArray &pixels);
    void updateAutoFrame(
        const QByteArray &pixels,
        const FrameStatistics &statistics
    );
    QImage createSmoothImage() const;
    QRgb smoothPixelColor(int outputX, int outputY) const;
    double bicubicNormalizedSample(
        double sourceX,
        double sourceY
    ) const;
    double cubicInterpolate(
        double p0,
        double p1,
        double p2,
        double p3,
        double fraction
    ) const;
    quint8 rawSourcePixel(int x, int y) const;
    double normalizedSourceValue(int x, int y) const;
    QRgb infernoColor(double normalizedValue) const;
    bool hasSentinelInBicubicNeighborhood(
        int anchorX,
        int anchorY
    ) const;
    QRgb nearestSourceColor(
        double sourceX,
        double sourceY
    ) const;

    // The most recently received thermal frame represented
    // as an indexed-color image.
    QImage m_image;

    // 256 RGB colors. Each incoming byte selects one entry.
    QVector<QRgb> m_colorTable;

    QByteArray m_rawPixels;
    FrameStatistics m_statistics;
    quint8 m_quantizationMode = 1;

    FrameModel::ScaleMode m_scaleMode =
        FrameModel::ScaleMode::Raw;
    bool m_smoothDisplay = false;
};

#endif
