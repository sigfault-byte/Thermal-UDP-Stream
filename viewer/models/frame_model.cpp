#include "frame_model.h"

#include <algorithm>

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

void FrameModel::setFrameTimingStatistics(
    const FrameTimingStatistics &statistics
)
{
    if (
        m_frameTimingStatistics.hasReceivedFrame
            == statistics.hasReceivedFrame
        &&
        m_frameTimingStatistics.hasReceivedFrameInterval
            == statistics.hasReceivedFrameInterval
        &&
        m_frameTimingStatistics.hasCameraFrameInterval
            == statistics.hasCameraFrameInterval
        &&
        m_frameTimingStatistics.isReceivingFrames
            == statistics.isReceivingFrames
        &&
        m_frameTimingStatistics.isStreamStale
            == statistics.isStreamStale
        &&
        m_frameTimingStatistics.receivedFrameIntervalMs
            == statistics.receivedFrameIntervalMs
        &&
        m_frameTimingStatistics.cameraFrameIntervalMs
            == statistics.cameraFrameIntervalMs
        &&
        m_frameTimingStatistics.receivedFramesPerSecond
            == statistics.receivedFramesPerSecond
    )
    {
        return;
    }

    m_frameTimingStatistics = statistics;

    emit frameTimingChanged();
}

bool FrameModel::hasReceivedFrame() const
{
    return m_frameTimingStatistics.hasReceivedFrame;
}

bool FrameModel::hasReceivedFrameInterval() const
{
    return m_frameTimingStatistics.hasReceivedFrameInterval;
}

bool FrameModel::hasCameraFrameInterval() const
{
    return m_frameTimingStatistics.hasCameraFrameInterval;
}

bool FrameModel::isReceivingFrames() const
{
    return m_frameTimingStatistics.isReceivingFrames;
}

bool FrameModel::isStreamStale() const
{
    return m_frameTimingStatistics.isStreamStale;
}

quint32 FrameModel::receivedFrameIntervalMs() const
{
    return m_frameTimingStatistics.receivedFrameIntervalMs;
}

quint32 FrameModel::cameraFrameIntervalMs() const
{
    return m_frameTimingStatistics.cameraFrameIntervalMs;
}

double FrameModel::receivedFramesPerSecond() const
{
    return m_frameTimingStatistics.receivedFramesPerSecond;
}

void FrameModel::setHotspot(const Hotspot &hotspot)
{
    m_hotspot = hotspot;
    emit hotspotChanged();
}

bool FrameModel::hotspotValid() const
{
    return m_hotspot.valid;
}

int FrameModel::hotspotPeakX() const
{
    return m_hotspot.peakX;
}

int FrameModel::hotspotPeakY() const
{
    return m_hotspot.peakY;
}

double FrameModel::hotspotPeakTemperatureCelsius() const
{
    return m_hotspot.peakTemperatureCelsius;
}

bool FrameModel::hotspotPeakAboveRange() const
{
    return m_hotspot.peakAboveRange;
}

double FrameModel::hotspotCenterX() const
{
    return m_hotspot.centerX;
}

double FrameModel::hotspotCenterY() const
{
    return m_hotspot.centerY;
}

double FrameModel::hotspotRadiusPixels() const
{
    return m_hotspot.radiusPixels;
}

int FrameModel::hotspotHotPixelCount() const
{
    return m_hotspot.hotPixelCount;
}

int FrameModel::hotspotTotalPixelCount() const
{
    return m_hotspot.totalPixelCount;
}

double FrameModel::hotspotScore() const
{
    return m_hotspot.score;
}

const HotspotSettings &FrameModel::hotspotSettings() const
{
    return m_hotspotSettings;
}

double FrameModel::hotspotTemperatureToleranceCelsius() const
{
    return m_hotspotSettings.temperatureToleranceCelsius;
}

double FrameModel::hotspotColdPixelPenalty() const
{
    return m_hotspotSettings.coldPixelPenalty;
}

int FrameModel::hotspotMaximumRadiusPixels() const
{
    return m_hotspotSettings.maximumRadiusPixels;
}

void FrameModel::setHotspotTemperatureToleranceCelsius(
    double value
)
{
    const double clampedValue =
        std::clamp(value, 0.1, 5.0);

    if (
        qFuzzyCompare(
            m_hotspotSettings.temperatureToleranceCelsius,
            clampedValue
        )
    )
    {
        return;
    }

    m_hotspotSettings.temperatureToleranceCelsius =
        clampedValue;

    emit hotspotSettingsChanged();
}

void FrameModel::setHotspotColdPixelPenalty(double value)
{
    const double clampedValue =
        std::clamp(value, 0.0, 10.0);

    if (
        qFuzzyCompare(
            m_hotspotSettings.coldPixelPenalty,
            clampedValue
        )
    )
    {
        return;
    }

    m_hotspotSettings.coldPixelPenalty =
        clampedValue;

    emit hotspotSettingsChanged();
}

void FrameModel::setHotspotMaximumRadiusPixels(int value)
{
    const int clampedValue =
        std::clamp(value, 1, 20);

    if (
        m_hotspotSettings.maximumRadiusPixels
        == clampedValue
    )
    {
        return;
    }

    m_hotspotSettings.maximumRadiusPixels =
        clampedValue;

    emit hotspotSettingsChanged();
}
