#ifndef FRAME_TIMING_TRACKER_H
#define FRAME_TIMING_TRACKER_H

#include <QElapsedTimer>

#include "frame_timing_statistics.h"

class FrameTimingTracker
{
public:
    FrameTimingTracker();

    FrameTimingStatistics registerReceivedFrame(
        quint32 cameraTimestampMs
    );

    FrameTimingStatistics refresh();

    void reset();

private:
    void updateFreshness(
        qint64 currentReceivedMs
    );

    QElapsedTimer m_elapsedTimer;
    FrameTimingStatistics m_statistics;
    qint64 m_previousReceivedMs = 0;
    quint32 m_previousCameraTimestampMs = 0;

    static constexpr qint64 StaleThresholdMs = 2000;
};

#endif
