#ifndef RECEIVER_STATISTICS_TRACKER_H
#define RECEIVER_STATISTICS_TRACKER_H

#include "receiver_statistics.h"

class ReceiverStatisticsTracker
{
public:
    void registerDatagramReceived();

    void registerCompletedFrame(
        quint32 frameId
    );

    void registerDatagramRejected();

    const ReceiverStatistics &statistics() const;

    void reset();

private:
    ReceiverStatistics m_statistics;
};

#endif
