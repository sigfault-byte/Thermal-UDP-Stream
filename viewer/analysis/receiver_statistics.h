#ifndef RECEIVER_STATISTICS_H
#define RECEIVER_STATISTICS_H

#include <QtGlobal>

struct ReceiverStatistics
{
    quint64 receivedDatagramCount = 0;
    quint64 completedFrameCount = 0;
    quint64 rejectedDatagramCount = 0;

    quint32 latestFrameId = 0;
};

#endif
