// One VPN profile and the worker process (omicliagent) that runs it.

#pragma once

#include "ManagementClient.h"

#include <QDateTime>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>

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
    const QStringList &log() const
    {
        return log_;
    }
    bool isActive() const
    {
        return state_ != State::Disconnected && state_ != State::Failed;
    }

    // true if the profile itself contains redirect-gateway
    bool redirectsGateway() const;

    static QString stateText(State state);

    void start(const QString &workerPath, const QString &logDir);
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

  private:
    void setState(State state, const QString &detail = QString());
    void appendLog(const QString &line);
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
    QDateTime connectedSince_;
    QStringList log_;

    // CRV1 challenge from the last auth failure, answered with the next
    // password request
    QString dynamicChallenge_;

    QString exitEventName_;
    void *exitEvent_ = nullptr; // HANDLE
};
