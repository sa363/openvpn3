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

    void start(const QString &workerPath, const QString &logDir, const ProxySettings &proxy = ProxySettings());

    // how many logs of earlier sessions are kept per profile
    static constexpr int keptLogs = 5;
    // file of the current session's log, the earlier ones are <name>.1.log ... <name>.5.log
    static QString logFile(const QString &logDir, const QString &profile);
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

    QString exitEventName_;
    void *exitEvent_ = nullptr; // HANDLE
};
