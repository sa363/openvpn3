#include "ConnectionManager.h"

#include "CredentialStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace {

const char keyConfigDir[] = "configDir";
const char keyWorkerPath[] = "workerPath";
const char keyAutoConnect[] = "autoConnect";
const char keyUsernames[] = "usernames/";
const char keyPasswords[] = "passwords/";
const char keyRestore[] = "restore";
const char keyAutoReconnect[] = "autoReconnect";
const char keyRestoreOnStartup[] = "restoreOnStartup";
const char keyProxyHost[] = "proxy/host";
const char keyProxyPort[] = "proxy/port";

QString defaultConfigDir()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath("OpenVPN3/config");
}

QString defaultWorkerPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("omicliagent.exe");
}

// settings keys can't contain slashes
QString profileKey(QString name)
{
    return name.replace('/', '|');
}

} // namespace

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject(parent)
{
    agentWasRunning_ = Connection::agentRunning();
    agentTimer_.setInterval(3000);
    connect(&agentTimer_, &QTimer::timeout, this, &ConnectionManager::watchAgent);
    agentTimer_.start();
}

bool ConnectionManager::autoReconnect() const
{
    return QSettings().value(keyAutoReconnect, true).toBool();
}

void ConnectionManager::setAutoReconnect(bool enabled)
{
    QSettings().setValue(keyAutoReconnect, enabled);
    for (Connection *c : connections_)
        c->setAutoReconnect(enabled);
}

bool ConnectionManager::restoreOnStartup() const
{
    return QSettings().value(keyRestoreOnStartup, true).toBool();
}

void ConnectionManager::setRestoreOnStartup(bool enabled)
{
    QSettings().setValue(keyRestoreOnStartup, enabled);
}

// Profiles that are wanted up. Only the user and permanent failures remove
// them, not exiting the GUI, so they come back with the next start.
void ConnectionManager::rememberWanted(const QString &name, bool wanted)
{
    QSettings s;
    QStringList list = s.value(keyRestore).toStringList();
    list.removeAll(name);
    if (wanted)
        list.append(name);
    s.setValue(keyRestore, list);
}

void ConnectionManager::watchAgent()
{
    if (!Connection::workerNeedsAgent(workerPath()))
        return;
    const bool running = Connection::agentRunning();
    if (running && !agentWasRunning_)
    {
        // The service came back: its tunnels were torn down when it stopped,
        // so set them up again, and don't keep waiting connections waiting.
        for (Connection *c : connections_)
        {
            if (c->isWaitingForRetry())
                c->retryNow();
            else if (c->isActive())
                c->reconnectTunnel();
        }
    }
    agentWasRunning_ = running;
}

QString ConnectionManager::configDir() const
{
    return QSettings().value(keyConfigDir, defaultConfigDir()).toString();
}

void ConnectionManager::setConfigDir(const QString &dir)
{
    QSettings().setValue(keyConfigDir, dir);
    reloadProfiles();
}

QString ConnectionManager::workerPath() const
{
    return QSettings().value(keyWorkerPath, defaultWorkerPath()).toString();
}

void ConnectionManager::setWorkerPath(const QString &path)
{
    QSettings().setValue(keyWorkerPath, path);
}

QString ConnectionManager::logDir() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath("logs");
}

ProxySettings ConnectionManager::proxy() const
{
    QSettings s;
    ProxySettings p;
    p.host = s.value(keyProxyHost).toString();
    p.port = static_cast<quint16>(s.value(keyProxyPort, 0).toUInt());
    return p;
}

void ConnectionManager::setProxy(const ProxySettings &proxy)
{
    QSettings s;
    if (proxy.host.isEmpty() || !proxy.port)
    {
        s.remove(keyProxyHost);
        s.remove(keyProxyPort);
        return;
    }
    s.setValue(keyProxyHost, proxy.host);
    s.setValue(keyProxyPort, proxy.port);
}

QStringList ConnectionManager::autoConnectProfiles() const
{
    return QSettings().value(keyAutoConnect).toStringList();
}

void ConnectionManager::setAutoConnect(const QString &name, bool enabled)
{
    QStringList list = autoConnectProfiles();
    list.removeAll(name);
    if (enabled)
        list.append(name);
    QSettings().setValue(keyAutoConnect, list);
}

bool ConnectionManager::isAutoConnect(const QString &name) const
{
    return autoConnectProfiles().contains(name);
}

QString ConnectionManager::savedUsername(const QString &name) const
{
    return QSettings().value(keyUsernames + profileKey(name)).toString();
}

void ConnectionManager::setSavedUsername(const QString &name, const QString &username)
{
    if (username.isEmpty())
        QSettings().remove(keyUsernames + profileKey(name));
    else
        QSettings().setValue(keyUsernames + profileKey(name), username);
}

bool ConnectionManager::canSavePasswords() const
{
    return CredentialStore::isAvailable();
}

QString ConnectionManager::savedPassword(const QString &name) const
{
    return CredentialStore::unprotect(QSettings().value(keyPasswords + profileKey(name)).toByteArray());
}

void ConnectionManager::setSavedPassword(const QString &name, const QString &password)
{
    const QByteArray blob = password.isEmpty() ? QByteArray() : CredentialStore::protect(password);
    if (blob.isEmpty())
        QSettings().remove(keyPasswords + profileKey(name));
    else
        QSettings().setValue(keyPasswords + profileKey(name), blob);
}

bool ConnectionManager::hasSavedPassword(const QString &name) const
{
    return QSettings().contains(keyPasswords + profileKey(name));
}

void ConnectionManager::reloadProfiles()
{
    const QDir dir(configDir());
    dir.mkpath(".");

    // name -> path of the profiles currently on disk
    QMap<QString, QString> found;
    QDirIterator it(dir.absolutePath(), {"*.ovpn", "*.conf"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QString path = it.next();
        QString name = dir.relativeFilePath(path);
        name.chop(QFileInfo(path).suffix().size() + 1);
        found.insert(name, path);
    }

    // drop profiles that disappeared, unless they are in use
    for (int i = connections_.size() - 1; i >= 0; --i)
    {
        Connection *c = connections_[i];
        if (found.value(c->name()) != c->configPath() && !c->isActive())
        {
            connections_.removeAt(i);
            c->deleteLater();
        }
    }

    for (auto f = found.cbegin(); f != found.cend(); ++f)
        if (!find(f.key()))
            add(new Connection(f.key(), f.value(), this));

    std::sort(connections_.begin(), connections_.end(), [](Connection *a, Connection *b)
              { return a->name().compare(b->name(), Qt::CaseInsensitive) < 0; });
    emit profilesChanged();
}

QString ConnectionManager::importProfile(const QString &path, QString *error)
{
    const QFileInfo src(path);
    const QString name = src.completeBaseName();
    const QString dest = QDir(configDir()).filePath(src.fileName());
    if (QFileInfo::exists(dest))
    {
        *error = tr("A profile named \"%1\" already exists").arg(name);
        return QString();
    }
    QDir().mkpath(configDir());
    if (!QFile::copy(path, dest))
    {
        *error = tr("Cannot copy %1 to %2").arg(QDir::toNativeSeparators(path), QDir::toNativeSeparators(dest));
        return QString();
    }
    reloadProfiles();
    return name;
}

Connection *ConnectionManager::find(const QString &name) const
{
    for (Connection *c : connections_)
        if (c->name() == name)
            return c;
    return nullptr;
}

int ConnectionManager::activeCount() const
{
    int n = 0;
    for (const Connection *c : connections_)
        if (c->isActive())
            ++n;
    return n;
}

bool ConnectionManager::canConnect(Connection *c, QString *conflict) const
{
    // Only the profile files are checked here, options pushed by the
    // server are not known before connecting.
    if (!c->redirectsGateway())
        return true;
    for (const Connection *other : connections_)
    {
        if (other != c && other->isActive() && other->redirectsGateway())
        {
            *conflict = other->name();
            return false;
        }
    }
    return true;
}

void ConnectionManager::connectProfile(Connection *c)
{
    c->start(workerPath(), logDir(), proxy());
}

void ConnectionManager::disconnectAll()
{
    for (Connection *c : connections_)
        if (c->isActive())
            c->stop();
}

void ConnectionManager::shutdownAll()
{
    for (Connection *c : connections_)
        if (c->isActive())
            c->shutdown();
}

void ConnectionManager::startAutoConnect()
{
    QStringList names = autoConnectProfiles();
    if (restoreOnStartup())
    {
        for (const QString &name : QSettings().value(keyRestore).toStringList())
            if (!names.contains(name))
                names.append(name);
    }
    for (const QString &name : names)
    {
        Connection *c = find(name);
        QString conflict;
        if (c && !c->isActive() && canConnect(c, &conflict))
            connectProfile(c);
    }
}

void ConnectionManager::add(Connection *c)
{
    connections_.append(c);
    c->setAutoReconnect(autoReconnect());
    connect(c, &Connection::wantedChanged, this, [this, c](bool wanted)
            { rememberWanted(c->name(), wanted); });
    connect(c, &Connection::stateChanged, this, [this, c]()
            { emit connectionStateChanged(c); });
}
