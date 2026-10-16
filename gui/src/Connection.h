// One VPN profile and the worker process (omicliagent) that runs it.

#pragma once

#include "ManagementClient.h"

#include <QDateTime>
#include <QList>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>

// traffic rate at one point in time, bytes per second
struct TrafficSample
{
    qint64 time = 0; // ms since epoch
    double in = 0;
    double out = 0;
};

// HTTP proxy the worker connects through, empty host for none
struct ProxySettings
{
    QString host;
    quint16 port = 0;
};

class Connection : public QObject
{
    Q_OBJECT

  public:
    enum class State
    {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Disconnecting,
        Failed
    };
    Q_ENUM(State)

    Connection(const QString &name, const QString &configPath, QObject *parent = nullptr);
    ~Connection() override;

    const QString &name() const
    {
        return name_;
    }
    const QString &configPath() const
    {
        return configPath_;
    }
    State state() const
    {
        return state_;
    }
    const QString &stateDetail() const
    {
        return stateDetail_;
    }
    const OmiState &lastOmiState() const
    {
        return omiState_;
    }
    quint64 bytesIn() const
    {
        return bytesIn_;
    }
    quint64 bytesOut() const
    {
        return bytesOut_;
    }
    QDateTime connectedSince() const
    {
        return connectedSince_;
    }
    // traffic rates of the current session, oldest first
    const QList<TrafficSample> &trafficHistory() const
    {
        return traffic_;
    }
    const QStringList &log() const
    {
        return log_;
    }
    bool isActive() const
    {
        return state_ != State::Disconnected && state_ != State::Failed;
    }
    // process id of the worker, 0 if it is not running
    qint64 workerPid() const;

    // true if the profile itself contains redirect-gateway
    bool redirectsGateway() const;

    static QString stateText(State state);

    // Connect on behalf of the user: the connection is wanted until stop(),
    // and with auto reconnect it is started again after it failed.
    void start(const QString &workerPath, const QString &logDir, const ProxySettings &proxy = ProxySettings());
    // disconnect because the GUI exits; the connection stays wanted
    void shutdown();
    // set up the tunnel again, e.g. after the agent was restarted
    void reconnectTunnel();

    bool isWanted() const
    {
        return wanted_;
    }
    void setAutoReconnect(bool enabled)
    {
        autoReconnect_ = enabled;
    }
    bool isWaitingForRetry() const
    {
        return retryTimer_.isActive();
    }
    void retryNow();

    // the worker sets up tunnels through the ovpnagent service
    static bool workerNeedsAgent(const QString &workerPath);
    // the ovpnagent service serves its named pipe
    static bool agentRunning();

    // how many logs of earlier sessions are kept per profile
    static constexpr int keptLogs = 5;
    // file of the current session's log, the earlier ones are <name>.1.log ... <name>.5.log
    static QString logFile(const QString &logDir, const QString &profile);
    // disconnect on behalf of the user
    void stop();

    // answers to passwordNeeded()
    void provideCredentials(const OmiPasswordRequest &request,
                            const QString &username,
                            const QString &password,
                            const QString &challengeResponse);
    void cancelCredentials();

  signals:
    void stateChanged();
    void statsChanged();
    void logAppended(const QString &line);
    // dynamicChallenge is the text of a CRV1 challenge, empty otherwise
    void credentialsNeeded(const OmiPasswordRequest &request, const QString &dynamicChallenge);
    void webAuthRequested(const QString &url);
    void errorOccurred(const QString &message);
    // the user connected or disconnected, or the connection failed for good
    void wantedChanged(bool wanted);
    // the server rejected the credentials (not a dynamic challenge)
    void authenticationFailed(const QString &type);
    // Routes of this tunnel overlap routes of the tunnel of another worker:
    // each entry is "<our route> overlaps <its route>". otherPid is 0 when
    // the agent only reported how many more there are.
    void routeConflicts(qint64 otherPid, const QStringList &overlaps);

  private:
    void setState(State state, const QString &detail = QString());
    void appendLog(const QString &line);
    void checkRouteConflict(const QString &text);
    void onOmiState(const OmiState &st);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void launch();
    void stopWorker();
    void setWanted(bool wanted);
    // retry later if the connection is wanted and the error may go away
    void fail(const QString &reason, const QString &detail, bool permanent);
    void killWorker();
    void signalExitEvent();
    void closeExitEvent();

    QString name_;
    QString configPath_;
    QProcess *process_ = nullptr;
    ManagementClient omi_;
    QTimer stopTimer_;

    State state_ = State::Disconnected;
    QString stateDetail_;
    QString fatalError_;
    OmiState omiState_;
    quint64 bytesIn_ = 0;
    quint64 bytesOut_ = 0;
    QList<TrafficSample> traffic_;
    QDateTime connectedSince_;
    QStringList log_;

    // ROUTE CONFLICT lines of the agent, reported together
    QMap<qint64, QStringList> pendingConflicts_;
    QTimer conflictTimer_;

    // CRV1 challenge from the last auth failure, answered with the next
    // password request
    QString dynamicChallenge_;

    // the user wants the connection up, see start() and stop()
    bool wanted_ = false;
    bool autoReconnect_ = true;
    // stop() or shutdown() asked the worker to exit
    bool stopRequested_ = false;
    int retries_ = 0;
    QTimer retryTimer_;
    QString workerPath_;
    QString logDir_;
    ProxySettings proxy_;
    // the error as the worker sent it, fatalError_ may be a friendlier text
    QString fatalRaw_;
    // reason of a halt pushed by the server
    QString haltReason_;

    QString exitEventName_;
    void *exitEvent_ = nullptr; // HANDLE
};
