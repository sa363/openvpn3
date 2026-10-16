#include "TrayController.h"

#include "AuthDialog.h"
#include "Connection.h"
#include "ConnectionManager.h"
#include "ConnectionsWindow.h"
#include "Icons.h"
#include "SettingsDialog.h"
#include "StatusWindow.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QUrl>

TrayController::TrayController(ConnectionManager *manager, QObject *parent)
    : QObject(parent),
      manager_(manager)
{
    menu_ = new QMenu;
    // rebuilt on every show, so it always reflects the current state
    connect(menu_, &QMenu::aboutToShow, this, &TrayController::rebuildMenu);
    rebuildMenu();

    tray_.setContextMenu(menu_);
    connect(&tray_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason)
            {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            showConnections(); });
    connect(&tray_, &QSystemTrayIcon::messageClicked, this, &TrayController::showConnections);

    connect(manager_, &ConnectionManager::profilesChanged, this, [this]()
            {
        for (Connection *c : manager_->connections())
            watch(c);
        updateTray(); });
    connect(manager_, &ConnectionManager::connectionStateChanged, this, &TrayController::onStateChanged);

    for (Connection *c : manager_->connections())
        watch(c);
    updateTray();
    tray_.show();
}

TrayController::~TrayController()
{
    delete menu_;
    delete connectionsWindow_;
    for (const auto &w : std::as_const(statusWindows_))
        delete w;
}

void TrayController::showConnections()
{
    if (!connectionsWindow_)
    {
        connectionsWindow_ = new ConnectionsWindow(manager_);
        connect(connectionsWindow_, &ConnectionsWindow::connectRequested, this, &TrayController::requestConnect);
        connect(connectionsWindow_, &ConnectionsWindow::statusRequested, this, &TrayController::showStatus);
        connect(connectionsWindow_, &ConnectionsWindow::importRequested, this, [this]()
                { importProfile(); });
        connect(connectionsWindow_, &ConnectionsWindow::settingsRequested, this, &TrayController::showSettings);
    }
    connectionsWindow_->show();
    connectionsWindow_->raise();
    connectionsWindow_->activateWindow();
}

void TrayController::requestConnect(Connection *c)
{
    if (c->isActive())
    {
        showStatus(c);
        return;
    }

    QString conflict;
    if (!manager_->canConnect(c, &conflict))
    {
        QMessageBox box(QMessageBox::Warning,
                        tr("Default gateway conflict"),
                        tr("Both \"%1\" and the active connection \"%2\" redirect all traffic "
                           "(redirect-gateway). Only one of them can be the default route.")
                            .arg(c->name(), conflict));
        // the agent refuses a second tunnel that redirects the default gateway,
        // so there is no point in connecting anyway
        QPushButton *replace = box.addButton(tr("Disconnect \"%1\"").arg(conflict), QMessageBox::AcceptRole);
        box.addButton(QMessageBox::Cancel);
        box.exec();
        if (box.clickedButton() == replace)
        {
            Connection *other = manager_->find(conflict);
            if (!other)
                return;
            // connect once the other tunnel is down
            auto *conn = new QMetaObject::Connection;
            *conn = connect(other, &Connection::stateChanged, this, [this, other, c, conn]()
                            {
                if (other->isActive())
                    return;
                disconnect(*conn);
                delete conn;
                manager_->connectProfile(c); });
            other->stop();
        }
        return;
    }

    manager_->connectProfile(c);
    if (c->state() == Connection::State::Failed)
        showStatus(c);
}

void TrayController::showStatus(Connection *c)
{
    QPointer<StatusWindow> &w = statusWindows_[c];
    if (!w)
    {
        w = new StatusWindow(manager_, c);
        connect(w, &StatusWindow::connectRequested, this, &TrayController::requestConnect);
        connect(c, &QObject::destroyed, w, &QObject::deleteLater);
    }
    w->show();
    w->raise();
    w->activateWindow();
}

void TrayController::importProfile(const QString &path)
{
    QString fn = path;
    if (fn.isEmpty())
        fn = QFileDialog::getOpenFileName(nullptr, tr("Import profile"), QDir::homePath(),
                                          tr("OpenVPN profiles (*.ovpn *.conf);;All files (*)"));
    if (fn.isEmpty())
        return;

    QString error;
    const QString name = manager_->importProfile(fn, &error);
    if (name.isEmpty())
        QMessageBox::warning(nullptr, tr("Import profile"), error);
    else
        tray_.showMessage(tr("Profile imported"), name, Icons::forState(Connection::State::Disconnected));
}

void TrayController::showSettings()
{
    SettingsDialog dlg(manager_);
    dlg.exec();
}

void TrayController::rebuildMenu()
{
    menu_->clear();

    const QList<Connection *> &conns = manager_->connections();
    if (conns.isEmpty())
    {
        menu_->addAction(tr("No profiles"))->setEnabled(false);
    }
    for (Connection *c : conns)
    {
        QMenu *sub = menu_->addMenu(Icons::forState(c->state()), c->name());
        sub->addAction(tr("Connect"), this, [this, c]()
                       { requestConnect(c); })
            ->setEnabled(!c->isActive());
        sub->addAction(tr("Disconnect"), c, &Connection::stop)
            ->setEnabled(c->isActive() && c->state() != Connection::State::Disconnecting);
        sub->addAction(tr("Show status"), this, [this, c]()
                       { showStatus(c); });
    }

    menu_->addSeparator();
    menu_->addAction(tr("Connections..."), this, &TrayController::showConnections);
    menu_->addAction(tr("Disconnect all"), manager_, &ConnectionManager::disconnectAll)
        ->setEnabled(manager_->activeCount() > 0);
    menu_->addSeparator();
    menu_->addAction(tr("Import profile..."), this, [this]()
                     { importProfile(); });
    menu_->addAction(tr("Reload profiles"), manager_, &ConnectionManager::reloadProfiles);
    menu_->addAction(tr("Settings..."), this, &TrayController::showSettings);
    menu_->addSeparator();
    menu_->addAction(tr("Exit"), this, &TrayController::quit);
}

void TrayController::updateTray()
{
    const QList<Connection *> &conns = manager_->connections();
    tray_.setIcon(Icons::forConnections(conns));

    QStringList lines{tr("OpenVPN3")};
    for (const Connection *c : conns)
    {
        if (!c->isActive())
            continue;
        QString line = c->name() + ": " + Connection::stateText(c->state());
        if (c->state() == Connection::State::Connected && !c->lastOmiState().localIp.isEmpty())
            line += " (" + c->lastOmiState().localIp + ')';
        lines << line;
    }
    if (lines.size() == 1)
        lines << tr("Not connected");
    tray_.setToolTip(lines.join('\n'));
}

void TrayController::watch(Connection *c)
{
    // connections are watched once, a property marks the ones already done
    if (c->property("ovpn3Watched").toBool())
        return;
    c->setProperty("ovpn3Watched", true);

    connect(c, &Connection::credentialsNeeded, this, [this, c](const OmiPasswordRequest &req, const QString &challenge)
            { askCredentials(c, req, challenge); });
    connect(c, &Connection::webAuthRequested, this, [this, c](const QString &url)
            {
        const QUrl u(url);
        if (u.scheme() != "https" && u.scheme() != "http")
        {
            c->stop();
            QMessageBox::warning(nullptr, c->name(), tr("The server asked to open an unsupported URL:\n%1").arg(url));
            return;
        }
        const auto answer = QMessageBox::question(nullptr, c->name(),
                                                  tr("The server requires sign-in in a web browser:\n%1\n\nOpen it now?").arg(url));
        if (answer == QMessageBox::Yes)
            QDesktopServices::openUrl(u);
        else
            c->stop(); });
    connect(c, &Connection::authenticationFailed, this, [this, c](const QString &type)
            {
        if (type == "Auth")
            manager_->setSavedPassword(c->name(), QString()); });
    connect(c, &Connection::errorOccurred, this, [this](const QString &message)
            { tray_.showMessage(tr("OpenVPN3"), message, QSystemTrayIcon::Warning); });
}

void TrayController::askCredentials(Connection *c, const OmiPasswordRequest &request, const QString &dynamicChallenge)
{
    // the username/password prompt, as opposed to a challenge or proxy credentials
    const bool authPrompt = request.type == "Auth" && dynamicChallenge.isEmpty();
    const QString savedUsername = manager_->savedUsername(c->name());
    const QString savedPassword = authPrompt ? manager_->savedPassword(c->name()) : QString();

    // sign in without asking when nothing has to be typed in
    if (authPrompt && !savedPassword.isEmpty() && request.staticChallenge.isEmpty()
        && (request.passwordOnly || !savedUsername.isEmpty()))
    {
        c->provideCredentials(request, savedUsername, savedPassword, QString());
        return;
    }

    auto *dlg = new AuthDialog(c->name(), request, dynamicChallenge, savedUsername, savedPassword,
                               manager_->canSavePasswords());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(c, &Connection::stateChanged, dlg, [dlg, c]()
            {
        // the connection went away while the dialog was open
        if (!c->isActive())
            dlg->reject(); });
    connect(dlg, &QDialog::finished, this, [this, dlg, c, request, authPrompt](int result)
            {
        if (result != QDialog::Accepted)
        {
            if (c->isActive())
                c->cancelCredentials();
            return;
        }
        if (!request.passwordOnly && request.type == "Auth")
            manager_->setSavedUsername(c->name(), dlg->rememberUsername() ? dlg->username() : QString());
        // saved before it is verified, an authentication failure removes it again
        if (authPrompt)
            manager_->setSavedPassword(c->name(), dlg->rememberPassword() ? dlg->password() : QString());
        c->provideCredentials(request, dlg->username(), dlg->password(), dlg->challengeResponse()); });
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void TrayController::onStateChanged(Connection *c)
{
    updateTray();
    if (c->state() == Connection::State::Connected)
    {
        const QString ip = c->lastOmiState().localIp;
        const QString msg = ip.isEmpty() ? tr("%1 is connected").arg(c->name())
                                         : tr("%1 is connected, address %2").arg(c->name(), ip);
        tray_.showMessage(tr("OpenVPN3"), msg, Icons::forState(Connection::State::Connected));
    }
}

void TrayController::quit()
{
    if (manager_->activeCount() > 0)
    {
        const auto answer = QMessageBox::question(nullptr, tr("Exit"),
                                                  tr("%n connection(s) are active. Disconnect and exit?", nullptr, manager_->activeCount()));
        if (answer != QMessageBox::Yes)
            return;
        manager_->disconnectAll();
    }

    // wait for the workers to tear their tunnels down
    auto *timer = new QTimer(this);
    auto *elapsed = new QElapsedTimer;
    elapsed->start();
    connect(timer, &QTimer::timeout, this, [this, timer, elapsed]()
            {
        if (manager_->activeCount() == 0 || elapsed->elapsed() > 8000)
        {
            timer->stop();
            delete elapsed;
            tray_.hide();
            QApplication::quit();
        } });
    timer->start(100);
}
