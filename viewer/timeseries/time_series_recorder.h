#ifndef TIME_SERIES_RECORDER_H
#define TIME_SERIES_RECORDER_H

#include "thermal_time_series_sample.h"

#include "detector/hotspot.h"
#include "analysis/frame_statistics.h"

#include <QAbstractListModel>
#include <QVector>

class TimeSeriesRecorder : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(
        bool recording
        READ isRecording
        NOTIFY recordingChanged
    )

    Q_PROPERTY(
        int sampleCount
        READ sampleCount
        NOTIFY sampleCountChanged
    )

public:
    enum Role
    {
        TimestampMsRole = Qt::UserRole + 1,

        MinimumCelsiusRole,
        MaximumCelsiusRole,
        MeanCelsiusRole,

        HotspotValidRole,
        HotspotPeakAboveRangeRole,
        HotspotPeakTemperatureCelsiusRole,
        HotspotCenterXRole,
        HotspotCenterYRole,
        HotspotRadiusPixelsRole,
        HotspotHotPixelCountRole,
        HotspotTotalPixelCountRole,
        HotspotScoreRole
    };

    explicit TimeSeriesRecorder(QObject *parent = nullptr);

    int rowCount(
        const QModelIndex &parent = QModelIndex()
    ) const override;

    QVariant data(
        const QModelIndex &index,
        int role
    ) const override;

    QHash<int, QByteArray> roleNames() const override;

    bool isRecording() const;
    int sampleCount() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void clear();

    void append(
        quint32 timestampMs,
        const FrameStatistics &statistics,
        const Hotspot &hotspot
    );

signals:
    void recordingChanged();
    void sampleCountChanged();

    void sampleAppended(
        quint32 timestampMs,
        double minimumCelsius,
        double maximumCelsius,
        double meanCelsius,
        bool hotspotValid,
        double hotspotPeakTemperatureCelsius
    );

private:
    bool m_recording = false;
    QVector<ThermalTimeSeriesSample> m_samples;
};

#endif
