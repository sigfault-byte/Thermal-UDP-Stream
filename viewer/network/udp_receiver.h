#ifndef UDP_RECEIVER_H
#define UDP_RECEIVER_H

#include <QFile>
#include <QObject>
#include <QString>
#include <QUdpSocket>
#include "analysis/receiver_statistics_tracker.h"
#include "protocol/thermal_frame.h"

class UdpReceiver : public QObject
{
    Q_OBJECT

public:
// create a UDP receiver that listens on the given local port.
    explicit UdpReceiver(
        // Qt’s fixed-width unsigned 16-bit integer type.
        // 0 and 65535 -> 16 bit :)
        quint16 port,
        const QString &rawSavePath = QString(),
        bool bindSocket = true,
        QObject *parent = nullptr
    );

    // Feed one complete raw packet into the same decoder used by UDP.
    // File replay calls this directly, so the app does not care where bytes came from.
    void processRawDatagram(
        const QByteArray &payload
    );

    //a slot is a member function intended to receive Qt signals
    // with modern function-pointer connect() syntax
    // this could simply be declared under private:
    // rather than private slots:
    // marking it as a slot is still useful documentation:
    // function exists specifically as a signal receiver.

    signals:
        // announce that one valid thermal frame was decoded.
        void thermalFrameReceived(const ThermalFrame &frame);
        //receiver stats signals
        void receiverStatisticsChanged(
            const ReceiverStatistics &statistics
        );

private slots:
    // called whenever one or more UDP datagrams are waiting.
    void processPendingDatagrams();

private:
    // the real UDP socket owned by this receiver object.
    QUdpSocket m_socket;
    ReceiverStatisticsTracker m_statisticsTracker;

    // When --save-bin is used, this file receives raw UDP payloads.
    // It is intentionally raw: even bad packets are saved before decode.
    QFile m_rawSaveFile;
};

#endif
