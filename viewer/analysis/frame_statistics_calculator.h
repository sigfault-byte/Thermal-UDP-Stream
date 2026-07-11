#ifndef FRAME_STATISTICS_CALCULATOR_H
#define FRAME_STATISTICS_CALCULATOR_H

#include <QByteArray>

#include "frame_statistics.h"

class FrameStatisticsCalculator
{
public:
    // Calculate statistics from valid thermal values.
    //
    // Protocol values 0 and 255 are reserved and therefore ignored.
    static FrameStatistics calculate(
        const QByteArray &pixels
    );
};

#endif
