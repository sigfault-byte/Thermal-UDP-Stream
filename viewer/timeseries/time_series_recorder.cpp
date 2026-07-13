#include "time_series_recorder.h"

TimeSeriesRecorder::TimeSeriesRecorder(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TimeSeriesRecorder::rowCount(
    const QModelIndex &parent
) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return m_samples.size();
}

void TimeSeriesRecorder::start()
{
    if (m_recording)
    {
        return;
    }

    m_recording = true;
    emit recordingChanged();
}

void TimeSeriesRecorder::stop()
{
    if (!m_recording)
    {
        return;
    }

    m_recording = false;
    emit recordingChanged();
}

void TimeSeriesRecorder::clear()
{
    if (m_samples.isEmpty())
    {
        return;
    }

    beginResetModel();
    m_samples.clear();
    endResetModel();

    emit sampleCountChanged();
}

bool TimeSeriesRecorder::isRecording() const
{
    return m_recording;
}

int TimeSeriesRecorder::sampleCount() const
{
    return m_samples.size();
}

void TimeSeriesRecorder::append(
    quint32 timestampMs,
    const FrameStatistics &statistics,
    const Hotspot &hotspot
)
{
    if (!m_recording)
    {
        return;
    }

    ThermalTimeSeriesSample sample;

    sample.timestampMs = timestampMs;

    sample.minimumCelsius =
        statistics.minimumCelsius;

    sample.maximumCelsius =
        statistics.maximumCelsius;

    sample.meanCelsius =
        statistics.meanCelsius;

    sample.hotspotValid =
        hotspot.valid;

    sample.hotspotPeakAboveRange =
        hotspot.peakAboveRange;

    sample.hotspotPeakTemperatureCelsius =
        hotspot.peakTemperatureCelsius;

    sample.hotspotCenterX =
        hotspot.centerX;

    sample.hotspotCenterY =
        hotspot.centerY;

    sample.hotspotRadiusPixels =
        hotspot.radiusPixels;

    sample.hotspotHotPixelCount =
        hotspot.hotPixelCount;

    sample.hotspotTotalPixelCount =
        hotspot.totalPixelCount;

    sample.hotspotScore =
        hotspot.score;

    const int newRow = m_samples.size();

    beginInsertRows(
        QModelIndex(),
        newRow,
        newRow
    );

    m_samples.append(sample);

    endInsertRows();

    emit sampleCountChanged();

    emit sampleAppended(
        sample.timestampMs,
        sample.minimumCelsius,
        sample.maximumCelsius,
        sample.meanCelsius,
        sample.hotspotValid,
        sample.hotspotPeakTemperatureCelsius
    );
}

QVariant TimeSeriesRecorder::data(
    const QModelIndex &index,
    int role
) const
{
    if (!index.isValid())
    {
        return {};
    }

    if (
        index.row() < 0
        || index.row() >= m_samples.size()
    )
    {
        return {};
    }

    const ThermalTimeSeriesSample &sample =
        m_samples[index.row()];

    switch (role)
    {
        case TimestampMsRole:
            return sample.timestampMs;

        case MinimumCelsiusRole:
            return sample.minimumCelsius;

        case MaximumCelsiusRole:
            return sample.maximumCelsius;

        case MeanCelsiusRole:
            return sample.meanCelsius;

        case HotspotValidRole:
            return sample.hotspotValid;

        case HotspotPeakAboveRangeRole:
            return sample.hotspotPeakAboveRange;

        case HotspotPeakTemperatureCelsiusRole:
            return sample.hotspotPeakTemperatureCelsius;

        case HotspotCenterXRole:
            return sample.hotspotCenterX;

        case HotspotCenterYRole:
            return sample.hotspotCenterY;

        case HotspotRadiusPixelsRole:
            return sample.hotspotRadiusPixels;

        case HotspotHotPixelCountRole:
            return sample.hotspotHotPixelCount;

        case HotspotTotalPixelCountRole:
            return sample.hotspotTotalPixelCount;

        case HotspotScoreRole:
            return sample.hotspotScore;

        default:
            return {};
    }
}

QHash<int, QByteArray>
TimeSeriesRecorder::roleNames() const
{
    return {
        {
            TimestampMsRole,
            "timestampMs"
        },
        {
            MinimumCelsiusRole,
            "minimumCelsius"
        },
        {
            MaximumCelsiusRole,
            "maximumCelsius"
        },
        {
            MeanCelsiusRole,
            "meanCelsius"
        },
        {
            HotspotValidRole,
            "hotspotValid"
        },
        {
            HotspotPeakAboveRangeRole,
            "hotspotPeakAboveRange"
        },
        {
            HotspotPeakTemperatureCelsiusRole,
            "hotspotPeakTemperatureCelsius"
        },
        {
            HotspotCenterXRole,
            "hotspotCenterX"
        },
        {
            HotspotCenterYRole,
            "hotspotCenterY"
        },
        {
            HotspotRadiusPixelsRole,
            "hotspotRadiusPixels"
        },
        {
            HotspotHotPixelCountRole,
            "hotspotHotPixelCount"
        },
        {
            HotspotTotalPixelCountRole,
            "hotspotTotalPixelCount"
        },
        {
            HotspotScoreRole,
            "hotspotScore"
        }
    };
}
