#include "receiver_statistics_tracker.h"

void ReceiverStatisticsTracker::registerDatagramReceived()
{
    ++m_statistics.receivedDatagramCount;
}

void ReceiverStatisticsTracker::registerCompletedFrame(
    quint32 frameId
)
{
    ++m_statistics.completedFrameCount;

    m_statistics.latestFrameId =
        frameId;
}

void ReceiverStatisticsTracker::registerDatagramRejected()
{
    ++m_statistics.rejectedDatagramCount;
}

const ReceiverStatistics &
ReceiverStatisticsTracker::statistics() const
{
    return m_statistics;
}

void ReceiverStatisticsTracker::reset()
{
    m_statistics = ReceiverStatistics {};
}
