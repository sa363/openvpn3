// OpenVPN3 GUI: tray application managing several simultaneous
// connections, each run by its own omicliagent worker process.

#include "ConnectionManager.h"
#include "TrayController.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLocale>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

QString instanceName()
{
    return "ovpn3-gui-" + qEnvironmentVariable("USERNAME", qEnvironmentVariable("USER"));
}

// Hand the request over to an already running instance.
bool forwardToRunningInstance(const QString &request)
{
    QLocalSocket socket;
    socket.connectToServer(instanceName());
    if (!socket.waitForConnected(500))
        return false;
#ifdef Q_OS_WIN
    // let the running instance bring its window to the foreground
    ::AllowSetForegroundWindow(ASFW_ANY);
#endif
    socket.write(request.toUtf8() + '\n');
    socket.waitForBytesWritten(1000);
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("OpenVPN3");
    QApplication::setApplicationName("ovpn3-gui");
    QApplication::setApplicationDisplayName("OpenVPN3");
    QApplication::setApplicationVersion(OVPN3_GUI_VERSION);
    QApplication::setQuitOnLastWindowClosed(false);

    // the GUI's and Qt's own translations for the system language
    for (const QString &base : {QStringLiteral("ovpn3-gui"), QStringLiteral("qtbase")})
    {
        auto *translator = new QTranslator(&app);
        if (translator->load(QLocale(), base, "_", ":/i18n"))
            QApplication::installTranslator(translator);
        else
            delete translator;
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("OpenVPN3 GUI");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption importOpt("import", "Import a profile.", "file");
    QCommandLineOption connectOpt("connect", "Connect a profile by name.", "profile");
    parser.addOption(importOpt);
    parser.addOption(connectOpt);
    parser.process(app);

    QString request = "show";
    if (parser.isSet(importOpt))
        request = "import " + QDir(QDir::currentPath()).absoluteFilePath(parser.value(importOpt));
    else if (parser.isSet(connectOpt))
        request = "connect " + parser.value(connectOpt);

    if (forwardToRunningInstance(request))
        return 0;

    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QMessageBox::critical(nullptr, "OpenVPN3", QObject::tr("No system tray available."));
        return 1;
    }

    ConnectionManager manager;
    manager.reloadProfiles();
    TrayController tray(&manager);
    // e.g. at logoff; the workers are also killed with the GUI
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &manager, &ConnectionManager::shutdownAll);

    QLocalServer server;
    QLocalServer::removeServer(instanceName());
    server.setSocketOptions(QLocalServer::UserAccessOption);
    server.listen(instanceName());

    auto handleRequest = [&](const QString &req)
    {
        if (req.startsWith("import "))
            tray.importProfile(req.mid(7));
        else if (req.startsWith("connect "))
        {
            if (Connection *c = manager.find(req.mid(8)))
                tray.requestConnect(c);
        }
        else if (req == "show")
            tray.showConnections();
    };

    QObject::connect(&server, &QLocalServer::newConnection, &server, [&]()
                     {
        while (QLocalSocket *s = server.nextPendingConnection())
        {
            QObject::connect(s, &QLocalSocket::readyRead, s, [s, &handleRequest]()
                             {
                while (s->canReadLine())
                    handleRequest(QString::fromUtf8(s->readLine()).trimmed()); });
            QObject::connect(s, &QLocalSocket::disconnected, s, &QObject::deleteLater);
        } });

    QTimer::singleShot(0, &app, [&]()
                       {
        manager.startAutoConnect();
        if (request != "show")
            handleRequest(request); });

    return app.exec();
}
