// Tray icon and the glue between connections and windows: connect
// requests, credential prompts, web auth and notifications.

#pragma once

#include "ManagementClient.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSystemTrayIcon>

class Connection;
class ConnectionManager;
class ConnectionsWindow;
class QMenu;
class StatusWindow;

class TrayController : public QObject
{
    Q_OBJECT

  public:
    explicit TrayController(ConnectionManager *manager, QObject *parent = nullptr);
    ~TrayController() override;

    void showConnections();
    void requestConnect(Connection *c);
    void showStatus(Connection *c);
    void importProfile(const QString &path = QString());
    void showSettings();

  private:
    void rebuildMenu();
    void updateTray();
    void watch(Connection *c);
    void askCredentials(Connection *c, const OmiPasswordRequest &request, const QString &dynamicChallenge);
    void onStateChanged(Connection *c);
    void quit();

    ConnectionManager *manager_;
    QSystemTrayIcon tray_;
    QMenu *menu_ = nullptr;
    QPointer<ConnectionsWindow> connectionsWindow_;
    QHash<Connection *, QPointer<StatusWindow>> statusWindows_;
};
