// Profile discovery, settings and the set of connections.

#pragma once

#include "Connection.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

class ConnectionManager : public QObject
{
    Q_OBJECT

  public:
    explicit ConnectionManager(QObject *parent = nullptr);

    // settings
    QString configDir() const;
    void setConfigDir(const QString &dir);
    QString workerPath() const;
    void setWorkerPath(const QString &path);
    QString logDir() const;
    // HTTP proxy for all connections, empty host for none
    ProxySettings proxy() const;
    void setProxy(const ProxySettings &proxy);
    QStringList autoConnectProfiles() const;
    void setAutoConnect(const QString &name, bool enabled);
    bool isAutoConnect(const QString &name) const;
    // start failed connections again
    bool autoReconnect() const;
    void setAutoReconnect(bool enabled);
    // at startup, connect what was connected when the GUI last ran
    bool restoreOnStartup() const;
    void setRestoreOnStartup(bool enabled);
    QString savedUsername(const QString &name) const;
    void setSavedUsername(const QString &name, const QString &username);
    bool canSavePasswords() const;
    QString savedPassword(const QString &name) const;
    void setSavedPassword(const QString &name, const QString &password);
    bool hasSavedPassword(const QString &name) const;

    // rescan configDir() for *.ovpn files, keeping active connections
    void reloadProfiles();
    // copy a profile into configDir(), return the new profile name or an empty string
    QString importProfile(const QString &path, QString *error);

    const QList<Connection *> &connections() const
    {
        return connections_;
    }
    Connection *find(const QString &name) const;
    int activeCount() const;

    // Start the connection. Returns false and sets *conflict if another
    // active connection also redirects the default gateway.
    bool canConnect(Connection *c, QString *conflict) const;
    void connectProfile(Connection *c);
    // disconnect on behalf of the user
    void disconnectAll();
    // disconnect because the GUI exits, the connections are restored next time
    void shutdownAll();
    // connect the profiles to connect on startup and the ones to restore
    void startAutoConnect();

  signals:
    void profilesChanged();
    void connectionStateChanged(Connection *c);

  private:
    void add(Connection *c);
    void rememberWanted(const QString &name, bool wanted);
    void watchAgent();

    QList<Connection *> connections_;

    // the service is restarted e.g. by an upgrade, tunnels are set up again then
    QTimer agentTimer_;
    bool agentWasRunning_ = true;
};
