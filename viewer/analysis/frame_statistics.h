#ifndef FRAME_STATISTICS_H
#define FRAME_STATISTICS_H

#include <QtGlobal>

struct FrameStatistics
{
    quint8 minimumEncoded = 0;
    quint8 maximumEncoded = 0;

    double minimumCelsius = 0.0;
    double maximumCelsius = 0.0;
    double meanCelsius = 0.0;

    int inRangePixelCount = 0;
    int belowRangePixelCount = 0;
    int aboveRangePixelCount = 0;
};

#endif
