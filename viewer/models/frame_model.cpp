#include "frame_model.h"

FrameModel::FrameModel(QObject *parent)
    : QObject(parent)
{
}

int FrameModel::frameId() const
{
    return m_frameId;
}

void FrameModel::setFrameId(int frameId)
{
    // Do nothing if the value did not actually change.
    if (m_frameId == frameId)
        return;

    // Store the new frame ID.
    m_frameId = frameId;

    // Tell Qt that the frameId property changed.
    emit frameIdChanged();
}

int FrameModel::imageRevision() const
{
    // not a frame or timestamp -> this is for QML to use a different image url
    return m_imageRevision;
}

void FrameModel::notifyImageUpdated()
{
    ++m_imageRevision;
    emit imageRevisionChanged();
}

FrameModel::ScaleMode FrameModel::scaleMode() const
{
    return m_scaleMode;
}

void FrameModel::setScaleMode(ScaleMode mode)
{
    // Do not emit a change signal when the state is unchanged.
    if (m_scaleMode == mode)
        return;

    m_scaleMode = mode;

    emit scaleModeChanged();
}

double FrameModel::minimumCelsius() const
{
    return m_statistics.minimumCelsius;
}

double FrameModel::maximumCelsius() const
{
    return m_statistics.maximumCelsius;
}

double FrameModel::meanCelsius() const
{
    return m_statistics.meanCelsius;
}

int FrameModel::inRangePixelCount() const
{
    return m_statistics.inRangePixelCount;
}

int FrameModel::belowRangePixelCount() const
{
    return m_statistics.belowRangePixelCount;
}

int FrameModel::aboveRangePixelCount() const
{
    return m_statistics.aboveRangePixelCount;
}

quint32 FrameModel::timestampMs() const
{
    return m_timestampMs;
}

void FrameModel::setTimestampMs(
    quint32 timestampMs
)
{
    if (m_timestampMs == timestampMs)
        return;

    m_timestampMs = timestampMs;

    emit timestampMsChanged();
}

void FrameModel::setStatistics(
    const FrameStatistics &statistics
)
{
    /*
     * For now, emit once for every received frame.
     * statistics will normally differ between frames anyway,
     * and one shared signal updates all dependent QML bindings.
     */
    m_statistics = statistics;

    emit statisticsChanged();
}

void FrameModel::setReceiverStatistics(
    const ReceiverStatistics &statistics
)
{
    if (
        m_receiverStatistics.receivedDatagramCount
            == statistics.receivedDatagramCount
        &&
        m_receiverStatistics.completedFrameCount
            == statistics.completedFrameCount
        &&
        m_receiverStatistics.rejectedDatagramCount
            == statistics.rejectedDatagramCount
        &&
        m_receiverStatistics.latestFrameId
            == statistics.latestFrameId
    )
    {
        return;
    }

    m_receiverStatistics = statistics;

    emit receiverStatisticsChanged();
}

quint64 FrameModel::receivedDatagramCount() const
{
    return m_receiverStatistics.receivedDatagramCount;
}

quint64 FrameModel::completedFrameCount() const
{
    return m_receiverStatistics.completedFrameCount;
}

quint64 FrameModel::rejectedDatagramCount() const
{
    return m_receiverStatistics.rejectedDatagramCount;
}

quint32 FrameModel::latestReceivedFrameId() const
{
    return m_receiverStatistics.latestFrameId;
}
