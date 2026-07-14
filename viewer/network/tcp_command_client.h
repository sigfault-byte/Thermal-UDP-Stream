#ifndef TCP_COMMAND_CLIENT_H
#define TCP_COMMAND_CLIENT_H

#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

class TcpCommandClient : public QObject
{
    Q_OBJECT

    // True after discovery has prepared one camera command endpoint.
    Q_PROPERTY(
        bool hasEndpoint
        READ hasEndpoint
        NOTIFY endpointChanged
    )

    // QML-friendly IP address text for the discovered endpoint.
    Q_PROPERTY(
        QString endpointAddressText
        READ endpointAddressText
        NOTIFY endpointChanged
    )

    // QML-friendly TCP port value for the discovered endpoint.
    Q_PROPERTY(
        quint16 endpointPort
        READ endpointPort
        NOTIFY endpointChanged
    )

    // QML-friendly "ip:port" text for the discovered endpoint.
    Q_PROPERTY(
        QString endpointText
        READ endpointText
        NOTIFY endpointChanged
    )

    // True when QML is allowed to send START or STOP.
    Q_PROPERTY(
        bool canSendCommand
        READ canSendCommand
        NOTIFY commandStateChanged
    )

    // True while one command is waiting for an ESP32 response.
    Q_PROPERTY(
        bool commandPending
        READ commandPending
        NOTIFY commandStateChanged
    )

    // True after START has been acknowledged and before STOP is acknowledged.
    Q_PROPERTY(
        bool cameraRunning
        READ cameraRunning
        NOTIFY commandStateChanged
    )

    // QML-friendly text for the single header button.
    Q_PROPERTY(
        QString commandButtonText
        READ commandButtonText
        NOTIFY commandStateChanged
    )

    // QML-friendly color for the single header button.
    Q_PROPERTY(
        QString commandButtonColor
        READ commandButtonColor
        NOTIFY commandStateChanged
    )

public:
    // Construct the endpoint-state shell.
    // This class owns the TCP socket used for command packets.
    explicit TcpCommandClient(
        QObject *parent = nullptr
    );

    // Store the endpoint discovered from the UDP handshake receiver.
    // Repeated identical broadcasts are ignored here to avoid noisy signals.
    void setEndpoint(
        const QHostAddress &address,
        quint16 port
    );

    bool hasEndpoint() const;
    QHostAddress endpointAddress() const;
    QString endpointAddressText() const;
    quint16 endpointPort() const;
    QString endpointText() const;

    bool canSendCommand() const;
    bool commandPending() const;
    bool cameraRunning() const;
    QString commandButtonText() const;
    QString commandButtonColor() const;

    // Send the START command from QML.
    // The UI remains pending until the ESP32 response is received.
    Q_INVOKABLE void sendStartCommand();

    // Send the STOP command from QML.
    // The UI remains pending until the ESP32 response is received.
    Q_INVOKABLE void sendStopCommand();

    // Send the SET_QUANTIZATION command from QML.
    // mode must be 1, 2, or 3; the UDP frame mode remains the display truth.
    Q_INVOKABLE void sendSetQuantizationCommand(
        int mode
    );

signals:
    // Emitted only when the prepared endpoint is first set or actually changes.
    void endpointChanged();

    // Emitted whenever command button state or camera running state changes.
    void commandStateChanged();

private slots:
    // Called when the TCP socket has connected to the discovered endpoint.
    void handleConnected();

    // Called when response bytes are ready from the ESP32.
    void handleReadyRead();

    // Called when the ESP32 closes the TCP socket.
    void handleDisconnected();

    // Called when the socket reports a TCP error.
    void handleSocketError();

    // Called when a command waits too long for its response.
    void handleCommandTimeout();

private:
    void sendCommand(
        quint8 command,
        quint8 value = 0
    );

    void writePendingCommand();

    void completePendingCommandOk();

    void completePendingCommandFailure(
        const QString &message
    );

    bool m_hasEndpoint = false;
    QHostAddress m_endpointAddress;
    quint16 m_endpointPort = 0;

    // The TCP socket used for command requests and responses.
    QTcpSocket m_socket;

    // The timeout prevents the header button from staying grey forever.
    QTimer m_commandTimeoutTimer;

    // Response bytes are buffered because TCP is a stream, not packet based.
    QByteArray m_receiveBuffer;

    bool m_commandPending = false;
    quint8 m_pendingCommand = 0;
    quint8 m_pendingCommandValue = 0;
    bool m_cameraRunning = false;
};

#endif
