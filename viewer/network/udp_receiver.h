#ifndef UDP_RECEIVER_H
#define UDP_RECEIVER_H

#include <QObject>
#include <QUdpSocket>

class UdpReceiver : public QObject
{
    Q_OBJECT

public:
// create a UDP receiver that listens on the given local port.
    explicit UdpReceiver(
        // Qt’s fixed-width unsigned 16-bit integer type.
        // 0 and 65535 -> 16 bit :)
        quint16 port,
        QObject *parent = nullptr
    );

    //a slot is a member function intended to receive Qt signals
    // with modern function-pointer connect() syntax
    // this could simply be declared under private:
    // rather than private slots:
    // marking it as a slot is still useful documentation:
    // function exists specifically as a signal receiver.

private slots:
    // called whenever one or more UDP datagrams are waiting.
    void processPendingDatagrams();

private:
    // the real UDP socket owned by this receiver object.
    QUdpSocket m_socket;
};

#endif
