// Client side of the OpenVPN management interface (OMI) spoken by the
// omicli/omicliagent workers, see openvpn/omi/omi.hpp.

#pragma once

#include <QByteArray>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

struct OmiState
{
    qint64 time = 0;
    QString name;   // CONNECTING, WAIT, AUTH, GET_CONFIG, CONNECTED, RECONNECTING, ...
    QString detail; // event info, e.g. reconnect reason
    QString localIp;
    QString remoteIp;
    QString remotePort;
    QString localIp6;
};

struct OmiPasswordRequest
{
    QString type; // "Auth" or "HTTP Proxy"
    bool passwordOnly = false;
    QString staticChallenge;
    bool staticChallengeEcho = false;
};

class ManagementClient : public QObject
{
    Q_OBJECT

  public:
    explicit ManagementClient(QObject *parent = nullptr);

    // Connect to the worker listening on 127.0.0.1:port, retrying until it
    // is up or timeoutMs has passed.
    void connectToWorker(quint16 port, const QByteArray &password, int timeoutMs = 10000);
    void disconnectFromWorker();
    bool isReady() const;

    // The last unprefixed line that was not followed by a notification. The
    // worker sends its final error like that before it exits.
    QString lastPlainLine() const
    {
        return lastPlainLine_;
    }

    void sendUsername(const QString &type, const QString &username);
    void sendPassword(const QString &type, const QString &password);
    void releaseHold();
    void terminate();
    // reconnect, which also sets up the tunnel again
    void restart();

    // quote an argument of a management command
    static QString quote(const QString &arg);

  signals:
    void ready();
    void connectFailed(const QString &reason);
    void disconnected();
    void stateChanged(const OmiState &state);
    void logLine(qint64 time, const QString &flags, const QString &text);
    // further lines of a multi-line log message, or the worker's final error
    void logContinuation(const QString &text);
    void byteCount(quint64 bytesIn, quint64 bytesOut);
    void passwordNeeded(const OmiPasswordRequest &request);
    // info is the CRV1 dynamic challenge string, if any
    void authFailed(const QString &type, const QString &info);
    void fatal(const QString &message);
    void notify(const QString &message);
    void commandError(const QString &message);

  private:
    enum class History
    {
        None,
        Log,
        State
    };

    void tryConnect();
    void onConnected();
    void onReadyRead();
    void processLine(const QString &line);
    void processNotification(const QString &type, const QString &payload);
    void processLog(const QString &payload);
    void processState(const QString &payload);
    void processPassword(const QString &payload);
    void send(const QString &cmd, History history = History::None);

    QTcpSocket socket_;
    QTimer retryTimer_;
    quint16 port_ = 0;
    QByteArray password_;
    qint64 deadline_ = 0;
    QByteArray in_;
    bool ready_ = false;
    QString lastPlainLine_;

    // history dumps of "log on all" / "state on all" arrive as
    // unprefixed lines terminated by END, in the order of the commands
    QQueue<History> pendingHistory_;
};
