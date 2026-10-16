#include "Connection.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTcpServer>
#include <QUuid>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

constexpr int maxLogLines = 5000;
// 5 minutes of samples with the 2 s byte count interval
constexpr int maxTrafficSamples = 150;
constexpr int stopTimeoutMs = 5000;

quint16 findFreePort()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0))
        return 0;
    return server.serverPort();
}

QByteArray randomPassword()
{
    QByteArray bytes(16, Qt::Uninitialized);
    QRandomGenerator::system()->generate(bytes.begin(), bytes.end());
    return bytes.toHex();
}

QString fileSafe(QString s)
{
    static const QRegularExpression bad(R"([\\/:*?"<>|\s])");
    return s.replace(bad, "_");
}

// omicliagent sets up the tunnel through the ovpnagent service
bool workerNeedsAgent(const QString &workerPath)
{
    return QFileInfo(workerPath).baseName().compare("omicliagent", Qt::CaseInsensitive) == 0;
}

bool agentRunning()
{
#ifdef Q_OS_WIN
    // fails with ERROR_FILE_NOT_FOUND if nobody serves the pipe,
    // ERROR_SEM_TIMEOUT only means all its instances are busy
    if (::WaitNamedPipeW(L"\\\\.\\pipe\\ovpnagent", 0))
        return true;
    return ::GetLastError() != ERROR_FILE_NOT_FOUND;
#else
    return true;
#endif
}

} // namespace

Connection::Connection(const QString &name, const QString &configPath, QObject *parent)
    : QObject(parent),
      name_(name),
      configPath_(configPath)
{
    stopTimer_.setSingleShot(true);
    connect(&stopTimer_, &QTimer::timeout, this, [this]()
            {
        // graceful stop did not work, try the exit event, then kill
        if (process_ && process_->state() != QProcess::NotRunning)
        {
            appendLog(tr("Worker did not stop in time, terminating it"));
            killWorker();
        } });

    connect(&omi_, &ManagementClient::ready, this, [this]()
            { appendLog(tr("Management interface connected")); });
    connect(&omi_, &ManagementClient::connectFailed, this, [this](const QString &reason)
            {
        fatalError_ = tr("Cannot connect to the management interface: %1").arg(reason);
        appendLog(fatalError_);
        killWorker(); });
    connect(&omi_, &ManagementClient::disconnected, this, [this]()
            {
        if (state_ != State::Disconnecting)
            appendLog(tr("Management interface disconnected")); });
    connect(&omi_, &ManagementClient::stateChanged, this, &Connection::onOmiState);
    connect(&omi_, &ManagementClient::logLine, this, [this](qint64 time, const QString &flags, const QString &text)
            {
        const QString ts = QDateTime::fromSecsSinceEpoch(time).toString("yyyy-MM-dd hh:mm:ss");
        appendLog(ts + ' ' + (flags.isEmpty() ? QString() : flags + ' ') + text); });
    connect(&omi_, &ManagementClient::byteCount, this, [this](quint64 in, quint64 out)
            {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (!traffic_.isEmpty() || bytesIn_ || bytesOut_)
        {
            const qint64 last = traffic_.isEmpty() ? now - 2000 : traffic_.last().time;
            const double secs = qMax<qint64>(now - last, 1) / 1000.0;
            // counters restart from zero after a reconnect
            const double din = in >= bytesIn_ ? double(in - bytesIn_) : double(in);
            const double dout = out >= bytesOut_ ? double(out - bytesOut_) : double(out);
            traffic_.append({now, din / secs, dout / secs});
            while (traffic_.size() > maxTrafficSamples)
                traffic_.removeFirst();
        }
        else
            traffic_.append({now, 0, 0});
        bytesIn_ = in;
        bytesOut_ = out;
        emit statsChanged(); });
    connect(&omi_, &ManagementClient::passwordNeeded, this, [this](const OmiPasswordRequest &req)
            {
        // CRV1:flags:state_id:username_b64:challenge_text
        QString challengeText;
        if (!dynamicChallenge_.isEmpty())
            challengeText = dynamicChallenge_.section(':', 4);
        emit credentialsNeeded(req, challengeText); });
    connect(&omi_, &ManagementClient::authFailed, this, [this](const QString &type, const QString &info)
            {
        dynamicChallenge_ = info.startsWith("CRV1:") ? info : QString();
        if (dynamicChallenge_.isEmpty())
        {
            appendLog(tr("%1 authentication failed").arg(type));
            emit authenticationFailed(type);
            emit errorOccurred(tr("Authentication failed for %1").arg(name_));
        } });
    connect(&omi_, &ManagementClient::fatal, this, [this](const QString &message)
            {
        fatalError_ = message;
        appendLog(tr("FATAL: %1").arg(message));
        // every simultaneous connection needs an adapter of its own
        if (message.contains("cannot acquire TAP handle"))
        {
            fatalError_ = tr("All VPN adapters are in use. Each simultaneous connection needs its own adapter; "
                             "add one from an elevated prompt, e.g. with \"tapctl create --hwid ovpn-dco\" "
                             "from OpenVPN Community.");
            appendLog(fatalError_);
        }
        // the agent refused a second tunnel taking over the default route,
        // e.g. because the server pushed redirect-gateway
        else if (message.contains("redirect-gateway: the default gateway is already redirected"))
        {
            fatalError_ = tr("Another connection already redirects all traffic (redirect-gateway). "
                             "Only one connection can be the default route; disconnect the other one first.");
            appendLog(fatalError_);
        } });
    connect(&omi_, &ManagementClient::notify, this, [this](const QString &message)
            { appendLog(tr("NOTIFY: %1").arg(message)); });
    connect(&omi_, &ManagementClient::commandError, this, [this](const QString &message)
            { appendLog(tr("Management error: %1").arg(message)); });
}

Connection::~Connection()
{
    if (process_)
    {
        process_->disconnect(this);
        if (process_->state() != QProcess::NotRunning)
        {
            omi_.terminate();
            if (!process_->waitForFinished(2000))
                killWorker();
        }
    }
    closeExitEvent();
}

QString Connection::stateText(State state)
{
    switch (state)
    {
    case State::Disconnected:
        return tr("Disconnected");
    case State::Connecting:
        return tr("Connecting");
    case State::Connected:
        return tr("Connected");
    case State::Reconnecting:
        return tr("Reconnecting");
    case State::Disconnecting:
        return tr("Disconnecting");
    case State::Failed:
        return tr("Failed");
    }
    return QString();
}

bool Connection::redirectsGateway() const
{
    QFile f(configPath_);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    static const QRegularExpression re(R"(^\s*redirect-gateway\b)", QRegularExpression::MultilineOption);
    return re.match(QString::fromUtf8(f.readAll())).hasMatch();
}

QString Connection::logFile(const QString &logDir, const QString &profile)
{
    return QDir(logDir).filePath(fileSafe(profile) + ".log");
}

// keep the logs of the last sessions: name.log -> name.1.log -> ... -> name.5.log
static void rotateLogs(const QString &logFile)
{
    QString base = logFile;
    base.chop(4); // ".log"
    auto numbered = [&base](int n)
    { return base + '.' + QString::number(n) + ".log"; };
    QFile::remove(numbered(Connection::keptLogs));
    for (int n = Connection::keptLogs - 1; n >= 1; --n)
        QFile::rename(numbered(n), numbered(n + 1));
    QFile::rename(logFile, numbered(1));
}

void Connection::start(const QString &workerPath, const QString &logDir, const ProxySettings &proxy)
{
    if (isActive())
        return;

    if (!QFileInfo(workerPath).isExecutable())
    {
        fatalError_ = tr("Worker not found: %1").arg(QDir::toNativeSeparators(workerPath));
        appendLog(fatalError_);
        setState(State::Failed, fatalError_);
        emit errorOccurred(fatalError_);
        return;
    }

    if (workerNeedsAgent(workerPath) && !agentRunning())
    {
        fatalError_ = tr("The ovpnagent service is not running. Start it from an elevated prompt with "
                         "\"ovpnagent.exe install\" or \"ovpnagent.exe run\", or select omicli.exe in "
                         "Settings and run the GUI as administrator.");
        appendLog(fatalError_);
        setState(State::Failed, tr("ovpnagent service is not running"));
        emit errorOccurred(fatalError_);
        return;
    }

    const quint16 port = findFreePort();
    if (!port)
    {
        fatalError_ = tr("No free port for the management interface");
        setState(State::Failed, fatalError_);
        emit errorOccurred(fatalError_);
        return;
    }

    fatalError_.clear();
    dynamicChallenge_.clear();
    bytesIn_ = bytesOut_ = 0;
    traffic_.clear();
    omiState_ = OmiState();
    connectedSince_ = QDateTime();
    emit statsChanged();

    QDir().mkpath(logDir);
    const QString logFile = Connection::logFile(logDir, name_);
    rotateLogs(logFile);

    closeExitEvent();
    exitEventName_ = "ovpn3-gui-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
#ifdef Q_OS_WIN
    // the worker opens the same event and exits when it is signalled
    exitEvent_ = ::CreateEventW(nullptr, FALSE, FALSE, reinterpret_cast<LPCWSTR>(exitEventName_.utf16()));
#endif

    const QByteArray password = randomPassword();

    process_ = new QProcess(this);
    process_->setProgram(workerPath);
    QStringList args{"--config", QDir::toNativeSeparators(configPath_),
                     "--management", "127.0.0.1", QString::number(port), "stdin",
                     "--management-query-passwords",
                     "--management-hold",
                     "--exit-event-name", exitEventName_,
                     "--log", QDir::toNativeSeparators(logFile)};
    if (!proxy.host.isEmpty() && proxy.port)
    {
        // overrides any http-proxy of the profile; credentials are asked for when needed
        args << "--http-proxy-override" << proxy.host << QString::number(proxy.port);
    }
    process_->setArguments(args);
    process_->setWorkingDirectory(QFileInfo(configPath_).absolutePath());
    process_->setProcessChannelMode(QProcess::MergedChannels);
    connect(process_, &QProcess::readyRead, process_, [p = process_]()
            { p->readAll(); }); // everything useful goes to --log and the OMI
    connect(process_, &QProcess::finished, this, &Connection::onProcessFinished);
    connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err)
            {
        if (err == QProcess::FailedToStart)
        {
            fatalError_ = tr("Cannot start worker: %1").arg(process_->errorString());
            appendLog(fatalError_);
            onProcessFinished(-1, QProcess::CrashExit);
        } });

    log_.clear();
    appendLog(tr("Starting %1").arg(QDir::toNativeSeparators(workerPath)));
    if (!proxy.host.isEmpty() && proxy.port)
        appendLog(tr("Using HTTP proxy %1:%2").arg(proxy.host).arg(proxy.port));
    setState(State::Connecting, tr("Starting worker"));

    process_->start();
    // the management password is read from stdin
    process_->write(password + "\n");
    process_->closeWriteChannel();

    omi_.connectToWorker(port, password);
}

void Connection::stop()
{
    if (!process_ || process_->state() == QProcess::NotRunning)
    {
        setState(State::Disconnected);
        return;
    }
    if (state_ == State::Disconnecting)
        return;

    setState(State::Disconnecting);
    if (omi_.isReady())
        omi_.terminate();
    else
        signalExitEvent();
    stopTimer_.start(stopTimeoutMs);
}

void Connection::provideCredentials(const OmiPasswordRequest &request,
                                    const QString &username,
                                    const QString &password,
                                    const QString &challengeResponse)
{
    if (!request.passwordOnly)
        omi_.sendUsername(request.type, username);

    QString pw = password;
    if (!dynamicChallenge_.isEmpty() && request.type == "Auth")
    {
        // the worker matches the response with the saved CRV1 cookie
        pw = challengeResponse;
        dynamicChallenge_.clear();
    }
    else if (!request.staticChallenge.isEmpty())
    {
        pw = "SCRV1:" + QString::fromLatin1(password.toUtf8().toBase64())
             + ':' + QString::fromLatin1(challengeResponse.toUtf8().toBase64());
    }
    omi_.sendPassword(request.type, pw);
}

void Connection::cancelCredentials()
{
    appendLog(tr("Credentials entry cancelled"));
    stop();
}

void Connection::setState(State state, const QString &detail)
{
    if (state == state_ && detail == stateDetail_)
        return;
    if (state == State::Connected && state_ != State::Connected)
        connectedSince_ = QDateTime::currentDateTime();
    else if (state != State::Connected)
        connectedSince_ = QDateTime();
    state_ = state;
    stateDetail_ = detail;
    emit stateChanged();
}

void Connection::appendLog(const QString &line)
{
    log_.append(line);
    while (log_.size() > maxLogLines)
        log_.removeFirst();
    emit logAppended(line);
}

void Connection::onOmiState(const OmiState &st)
{
    if (state_ == State::Disconnecting)
        return;

    if (st.name == "INFO" || st.name == "AUTH_PENDING")
    {
        // web based authentication
        for (const QString prefix : {QStringLiteral("OPEN_URL:"), QStringLiteral("WEB_AUTH:")})
        {
            if (st.detail.startsWith(prefix))
            {
                QString url = st.detail.mid(prefix.size());
                // WEB_AUTH:flags:url
                if (prefix == "WEB_AUTH:")
                    url = url.section(':', 1);
                emit webAuthRequested(url);
                return;
            }
        }
        if (st.name == "INFO")
            return;
    }

    omiState_ = st;
    if (st.name == "CONNECTED")
        setState(State::Connected, st.localIp);
    else if (st.name == "RECONNECTING")
        setState(State::Reconnecting, st.detail);
    else if (st.name == "EXITING")
        setState(State::Disconnecting, st.detail);
    else if (st.name == "PAUSE")
        setState(State::Reconnecting, tr("Paused"));
    else
        setState(state_ == State::Reconnecting ? State::Reconnecting : State::Connecting, st.name);
}

void Connection::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    stopTimer_.stop();
    omi_.disconnectFromWorker();
    closeExitEvent();

    const bool requested = state_ == State::Disconnecting;
    appendLog(tr("Worker exited (code %1)").arg(exitCode));

    if (process_)
    {
        process_->deleteLater();
        process_ = nullptr;
    }

    if (requested && fatalError_.isEmpty())
        setState(State::Disconnected);
    else
    {
        const QString reason = fatalError_.isEmpty()
                                   ? (status == QProcess::CrashExit ? tr("Worker crashed") : tr("Worker exited unexpectedly"))
                                   : fatalError_;
        setState(State::Failed, reason);
        emit errorOccurred(tr("%1: %2").arg(name_, reason));
    }
}

void Connection::killWorker()
{
    signalExitEvent();
    if (process_ && process_->state() != QProcess::NotRunning)
    {
        if (!process_->waitForFinished(1000))
            process_->kill();
    }
}

void Connection::signalExitEvent()
{
#ifdef Q_OS_WIN
    if (exitEvent_)
        ::SetEvent(exitEvent_);
#endif
}

void Connection::closeExitEvent()
{
#ifdef Q_OS_WIN
    if (exitEvent_)
        ::CloseHandle(exitEvent_);
#endif
    exitEvent_ = nullptr;
}
