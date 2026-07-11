#ifndef FRAME_MODEL_H
#define FRAME_MODEL_H

#include <QObject>

#include "analysis/frame_statistics.h"

class FrameModel : public QObject
{
    Q_OBJECT

    // Expose frameId to QML as a read-only property.
    // QML reads it through frameId() and watches frameIdChanged().
    Q_PROPERTY(
        int frameId
        READ frameId
        NOTIFY frameIdChanged
    )
    Q_PROPERTY(
        int imageRevision
        READ imageRevision
        NOTIFY imageRevisionChanged
    )
    Q_PROPERTY(
        ScaleMode scaleMode
        READ scaleMode
        WRITE setScaleMode
        NOTIFY scaleModeChanged
    )

    //statistics
    Q_PROPERTY(
        double minimumCelsius
        READ minimumCelsius
        NOTIFY statisticsChanged
    )

    Q_PROPERTY(
        double maximumCelsius
        READ maximumCelsius
        NOTIFY statisticsChanged
    )

    Q_PROPERTY(
        double meanCelsius
        READ meanCelsius
        NOTIFY statisticsChanged
    )

    Q_PROPERTY(
        int inRangePixelCount
        READ inRangePixelCount
        NOTIFY statisticsChanged
    )

    Q_PROPERTY(
        int belowRangePixelCount
        READ belowRangePixelCount
        NOTIFY statisticsChanged
    )

    Q_PROPERTY(
        int aboveRangePixelCount
        READ aboveRangePixelCount
        NOTIFY statisticsChanged
    )

    Q_PROPERTY(
        quint32 timestampMs
        READ timestampMs
        NOTIFY timestampMsChanged
    )

public:
    // The enum belongs to FrameModel and is exposed to Qt/QML.
        enum class ScaleMode
        {
            Raw,
            Auto
        };
// Register it for qt
    Q_ENUM(ScaleMode)


    explicit FrameModel(QObject *parent = nullptr);

    // Getter used by Qt/QML to read the current frame ID.
    int frameId() const;
    // Update the current frame ID.
    void setFrameId(int frameId);

    // value used by QML to produce a new image URL whenever
    // the thermal image changes.
    int imageRevision() const;
    // Announce that the image provider now contains a new frame.
    void notifyImageUpdated();

    ScaleMode scaleMode() const;
    void setScaleMode(ScaleMode mode);

    double minimumCelsius() const;
    double maximumCelsius() const;
    double meanCelsius() const;

    int inRangePixelCount() const;
    int belowRangePixelCount() const;
    int aboveRangePixelCount() const;

    quint32 timestampMs() const;
    void setTimestampMs(quint32 timestampMs);

    // Replace all frame statistics and notify QML.
    void setStatistics(const FrameStatistics &statistics);

signals:
    // Emitted whenever frameId changes.
    // QML listens to this signal and updates dependent bindings.
    void frameIdChanged();
    void imageRevisionChanged();
    void scaleModeChanged();

    void statisticsChanged();
    void timestampMsChanged();

private:
    int m_frameId = 0;
    int m_imageRevision = 0;

    ScaleMode m_scaleMode = ScaleMode::Raw;

    FrameStatistics m_statistics;
    quint32 m_timestampMs = 0;
};

#endif
