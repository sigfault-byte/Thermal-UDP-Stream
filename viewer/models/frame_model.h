#ifndef FRAME_MODEL_H
#define FRAME_MODEL_H

#include <QObject>

#include "analysis/frame_statistics.h"
#include "analysis/frame_timing_statistics.h"
#include "analysis/receiver_statistics.h"

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

    Q_PROPERTY(
        quint64 receivedDatagramCount
        READ receivedDatagramCount
        NOTIFY receiverStatisticsChanged
    )

    Q_PROPERTY(
        quint64 completedFrameCount
        READ completedFrameCount
        NOTIFY receiverStatisticsChanged
    )

    Q_PROPERTY(
        quint64 rejectedDatagramCount
        READ rejectedDatagramCount
        NOTIFY receiverStatisticsChanged
    )

    Q_PROPERTY(
        quint32 latestReceivedFrameId
        READ latestReceivedFrameId
        NOTIFY receiverStatisticsChanged
    )

    Q_PROPERTY(
        bool hasReceivedFrame
        READ hasReceivedFrame
        NOTIFY frameTimingChanged
    )

    Q_PROPERTY(
        bool hasReceivedFrameInterval
        READ hasReceivedFrameInterval
        NOTIFY frameTimingChanged
    )

    Q_PROPERTY(
        bool hasCameraFrameInterval
        READ hasCameraFrameInterval
        NOTIFY frameTimingChanged
    )

    Q_PROPERTY(
        bool isReceivingFrames
        READ isReceivingFrames
        NOTIFY frameTimingChanged
    )

    Q_PROPERTY(
        bool isStreamStale
        READ isStreamStale
        NOTIFY frameTimingChanged
    )

    Q_PROPERTY(
        quint32 receivedFrameIntervalMs
        READ receivedFrameIntervalMs
        NOTIFY frameTimingChanged
    )

    Q_PROPERTY(
        quint32 cameraFrameIntervalMs
        READ cameraFrameIntervalMs
        NOTIFY frameTimingChanged
    )

    Q_PROPERTY(
        double receivedFramesPerSecond
        READ receivedFramesPerSecond
        NOTIFY frameTimingChanged
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

    // Replace all receiver statistics and notify QML.
    void setReceiverStatistics(const ReceiverStatistics &statistics);

    quint64 receivedDatagramCount() const;
    quint64 completedFrameCount() const;
    quint64 rejectedDatagramCount() const;
    quint32 latestReceivedFrameId() const;

    // Replace all frame timing values and notify QML.
    void setFrameTimingStatistics(
        const FrameTimingStatistics &statistics
    );

    bool hasReceivedFrame() const;
    bool hasReceivedFrameInterval() const;
    bool hasCameraFrameInterval() const;
    bool isReceivingFrames() const;
    bool isStreamStale() const;
    quint32 receivedFrameIntervalMs() const;
    quint32 cameraFrameIntervalMs() const;
    double receivedFramesPerSecond() const;

signals:
    // Emitted whenever frameId changes.
    // QML listens to this signal and updates dependent bindings.
    void frameIdChanged();
    void imageRevisionChanged();
    void scaleModeChanged();

    void statisticsChanged();
    void timestampMsChanged();
    void receiverStatisticsChanged();
    void frameTimingChanged();

private:
    int m_frameId = 0;
    int m_imageRevision = 0;

    ScaleMode m_scaleMode = ScaleMode::Raw;

    FrameStatistics m_statistics;
    ReceiverStatistics m_receiverStatistics;
    FrameTimingStatistics m_frameTimingStatistics;
    quint32 m_timestampMs = 0;
};

#endif
