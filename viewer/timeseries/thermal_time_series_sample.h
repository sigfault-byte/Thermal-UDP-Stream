#ifndef THERMAL_TIME_SERIES_SAMPLE_H
#define THERMAL_TIME_SERIES_SAMPLE_H

#include <QtGlobal>

struct ThermalTimeSeriesSample
{
    quint32 timestampMs = 0;

    double minimumCelsius = 0.0;
    double maximumCelsius = 0.0;
    double meanCelsius = 0.0;

    bool hotspotValid = false;
    bool hotspotPeakAboveRange = false;

    double hotspotPeakTemperatureCelsius = 0.0;
    double hotspotCenterX = 0.0;
    double hotspotCenterY = 0.0;
    double hotspotRadiusPixels = 0.0;

    int hotspotHotPixelCount = 0;
    int hotspotTotalPixelCount = 0;

    double hotspotScore = 0.0;
};

#endif
