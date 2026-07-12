#ifndef FRAME_TIMING_STATISTICS_H
#define FRAME_TIMING_STATISTICS_H

#include <QtGlobal>

struct FrameTimingStatistics
{
    bool hasReceivedFrame = false;
    bool hasReceivedFrameInterval = false;
    bool hasCameraFrameInterval = false;
    bool isReceivingFrames = false;
    bool isStreamStale = false;

    quint32 receivedFrameIntervalMs = 0;
    quint32 cameraFrameIntervalMs = 0;
    double receivedFramesPerSecond = 0.0;
};

#endif
