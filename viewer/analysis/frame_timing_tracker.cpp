#include "frame_timing_tracker.h"

FrameTimingTracker::FrameTimingTracker()
{
    m_elapsedTimer.start();
}

FrameTimingStatistics FrameTimingTracker::registerReceivedFrame(
    quint32 cameraTimestampMs
)
{
    if (!m_elapsedTimer.isValid())
        m_elapsedTimer.start();

    const qint64 receivedMs =
        m_elapsedTimer.elapsed();

    if (!m_statistics.hasReceivedFrame)
    {
        m_statistics.hasReceivedFrame = true;
        m_statistics.isReceivingFrames = true;
        m_statistics.isStreamStale = false;
        m_statistics.hasReceivedFrameInterval = false;
        m_statistics.hasCameraFrameInterval = false;
    }
    else if (m_statistics.isStreamStale)
    {
        m_statistics.isReceivingFrames = true;
        m_statistics.isStreamStale = false;
        m_statistics.hasReceivedFrameInterval = false;
        m_statistics.hasCameraFrameInterval = false;
        m_statistics.receivedFrameIntervalMs = 0;
        m_statistics.cameraFrameIntervalMs = 0;
        m_statistics.receivedFramesPerSecond = 0.0;
    }
    else
    {
        const qint64 receivedIntervalMs =
            receivedMs - m_previousReceivedMs;

        if (receivedIntervalMs > 0)
        {
            m_statistics.hasReceivedFrameInterval = true;
            m_statistics.receivedFrameIntervalMs =
                static_cast<quint32>(receivedIntervalMs);
            m_statistics.receivedFramesPerSecond =
                1000.0 / static_cast<double>(receivedIntervalMs);
        }
        else
        {
            m_statistics.hasReceivedFrameInterval = false;
            m_statistics.receivedFrameIntervalMs = 0;
            m_statistics.receivedFramesPerSecond = 0.0;
        }

        if (cameraTimestampMs > m_previousCameraTimestampMs)
        {
            m_statistics.hasCameraFrameInterval = true;
            m_statistics.cameraFrameIntervalMs =
                cameraTimestampMs - m_previousCameraTimestampMs;
        }
        else
        {
            m_statistics.hasCameraFrameInterval = false;
            m_statistics.cameraFrameIntervalMs = 0;
        }
    }

    m_previousReceivedMs = receivedMs;
    m_previousCameraTimestampMs = cameraTimestampMs;
    updateFreshness(
        receivedMs
    );

    return m_statistics;
}

FrameTimingStatistics FrameTimingTracker::refresh()
{
    if (!m_elapsedTimer.isValid())
        m_elapsedTimer.start();

    updateFreshness(
        m_elapsedTimer.elapsed()
    );

    return m_statistics;
}

void FrameTimingTracker::reset()
{
    m_elapsedTimer.restart();
    m_statistics = FrameTimingStatistics {};
    m_previousReceivedMs = 0;
    m_previousCameraTimestampMs = 0;
}

void FrameTimingTracker::updateFreshness(
    qint64 currentReceivedMs
)
{
    if (!m_statistics.hasReceivedFrame)
    {
        m_statistics.isReceivingFrames = false;
        m_statistics.isStreamStale = false;
        return;
    }

    const bool isStale =
        currentReceivedMs - m_previousReceivedMs > StaleThresholdMs;

    m_statistics.isReceivingFrames =
        !isStale;
    m_statistics.isStreamStale =
        isStale;

    if (isStale)
    {
        m_statistics.hasReceivedFrameInterval = false;
        m_statistics.hasCameraFrameInterval = false;
        m_statistics.receivedFrameIntervalMs = 0;
        m_statistics.cameraFrameIntervalMs = 0;
        m_statistics.receivedFramesPerSecond = 0.0;
    }
}
