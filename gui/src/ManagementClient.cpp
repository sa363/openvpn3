#include "ManagementClient.h"

#include <QDateTime>
#include <QHostAddress>
#include <QRegularExpression>

ManagementClient::ManagementClient(QObject *parent)
    : QObject(parent)
{
    retryTimer_.setSingleShot(true);
    retryTimer_.setInterval(250);
    connect(&retryTimer_, &QTimer::timeout, this, &ManagementClient::tryConnect);

    connect(&socket_, &QTcpSocket::connected, this, &ManagementClient::onConnected);
    connect(&socket_, &QTcpSocket::readyRead, this, &ManagementClient::onReadyRead);
    connect(&socket_, &QTcpSocket::disconnected, this, [this]()
            {
        const bool wasReady = ready_;
        ready_ = false;
        pendingHistory_.clear();
        in_.clear();
        if (wasReady)
            emit disconnected(); });
    connect(&socket_, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError)
            {
        if (ready_)
            return; // handled by disconnected()
        if (QDateTime::currentMSecsSinceEpoch() < deadline_)
            retryTimer_.start();
        else
            emit connectFailed(socket_.errorString()); });
}

void ManagementClient::connectToWorker(quint16 port, const QByteArray &password, int timeoutMs)
{
    disconnectFromWorker();
    port_ = port;
    password_ = password;
    deadline_ = QDateTime::currentMSecsSinceEpoch() + timeoutMs;
    tryConnect();
}

void ManagementClient::disconnectFromWorker()
{
    retryTimer_.stop();
    ready_ = false;
    socket_.abort();
    in_.clear();
    pendingHistory_.clear();
}

bool ManagementClient::isReady() const
{
    return ready_;
}

void ManagementClient::tryConnect()
{
    socket_.abort();
    socket_.connectToHost(QHostAddress::LocalHost, port_);
}

void ManagementClient::onConnected()
{
    // wait for "ENTER PASSWORD:" or the >INFO banner
}

void ManagementClient::sendUsername(const QString &type, const QString &username)
{
    send("username " + quote(type) + ' ' + quote(username));
}

void ManagementClient::sendPassword(const QString &type, const QString &password)
{
    send("password " + quote(type) + ' ' + quote(password));
}

void ManagementClient::releaseHold()
{
    send("hold release");
}

void ManagementClient::terminate()
{
    send("signal SIGTERM");
}

QString ManagementClient::quote(const QString &arg)
{
    QString ret = "\"";
    for (const QChar c : arg)
    {
        if (c == '\\' || c == '"')
            ret += '\\';
        ret += c;
    }
    ret += '"';
    return ret;
}

void ManagementClient::send(const QString &cmd, History history)
{
    if (socket_.state() != QAbstractSocket::ConnectedState)
        return;
    if (history != History::None)
        pendingHistory_.enqueue(history);
    socket_.write(cmd.toUtf8() + "\r\n");
}

void ManagementClient::onReadyRead()
{
    in_ += socket_.readAll();

    // the password prompt is not terminated by a newline
    static const QByteArray prompt = "ENTER PASSWORD:";
    if (in_.startsWith(prompt))
    {
        in_.remove(0, prompt.size());
        socket_.write(password_ + "\r\n");
    }

    qsizetype pos;
    while ((pos = in_.indexOf('\n')) >= 0)
    {
        QByteArray line = in_.left(pos);
        in_.remove(0, pos + 1);
        if (line.endsWith('\r'))
            line.chop(1);
        processLine(QString::fromUtf8(line));
    }
}

void ManagementClient::processLine(const QString &line)
{
    if (line.startsWith('>'))
    {
        const qsizetype colon = line.indexOf(':');
        if (colon < 0)
            return;
        processNotification(line.mid(1, colon - 1), line.mid(colon + 1));
        return;
    }

    if (line.startsWith("SUCCESS:"))
        return;

    if (line.startsWith("ERROR:"))
    {
        emit commandError(line.mid(6).trimmed());
        return;
    }

    if (pendingHistory_.isEmpty())
        return;

    if (line == "END")
    {
        pendingHistory_.dequeue();
        return;
    }

    if (pendingHistory_.head() == History::Log)
        processLog(line);
    else
        processState(line);
}

void ManagementClient::processNotification(const QString &type, const QString &payload)
{
    if (type == "INFO")
    {
        if (!ready_ && payload.startsWith("OpenVPN Management Interface"))
        {
            ready_ = true;
            retryTimer_.stop();
            send("log on all", History::Log);
            send("state on all", History::State);
            send("bytecount 2");
            emit ready();
        }
    }
    else if (type == "HOLD")
        releaseHold();
    else if (type == "LOG")
        processLog(payload);
    else if (type == "STATE")
        processState(payload);
    else if (type == "BYTECOUNT")
    {
        const QStringList f = payload.split(',');
        if (f.size() >= 2)
            emit byteCount(f[0].toULongLong(), f[1].toULongLong());
    }
    else if (type == "PASSWORD")
        processPassword(payload);
    else if (type == "FATAL")
        emit fatal(payload.trimmed());
    else if (type == "NOTIFY")
        emit notify(payload);
}

// time,flags,text
void ManagementClient::processLog(const QString &payload)
{
    const QStringList f = payload.split(',');
    if (f.size() < 3)
        return;
    emit logLine(f[0].toLongLong(), f[1], payload.section(',', 2));
}

// time,NAME,info,, or time,CONNECTED,SUCCESS,vpnIp4,serverIp,serverPort,,,vpnIp6
void ManagementClient::processState(const QString &payload)
{
    const QStringList f = payload.split(',');
    if (f.size() < 2)
        return;

    OmiState st;
    st.time = f[0].toLongLong();
    st.name = f[1];
    st.detail = f.value(2);
    if (st.name == "CONNECTED")
    {
        st.localIp = f.value(3);
        st.remoteIp = f.value(4);
        st.remotePort = f.value(5);
        st.localIp6 = f.value(8);
    }
    else if (st.name == "INFO" || st.name == "AUTH_PENDING")
    {
        // info may itself contain commas (URLs)
        st.detail = payload.section(',', 2);
        while (st.detail.endsWith(','))
            st.detail.chop(1);
    }
    emit stateChanged(st);
}

void ManagementClient::processPassword(const QString &payload)
{
    // Need 'Auth' username/password [SC:1,challenge text]
    static const QRegularExpression needRe(R"(^Need '([^']+)' (username/password|password)(?: SC:([01]),(.*))?$)");
    // Verification Failed: 'Auth' ['CRV1:...']
    static const QRegularExpression failedRe(R"(^Verification Failed: '([^']+)'(?: \['(.*)'\])?$)");

    const QRegularExpressionMatch need = needRe.match(payload);
    if (need.hasMatch())
    {
        OmiPasswordRequest req;
        req.type = need.captured(1);
        req.passwordOnly = need.captured(2) == "password";
        req.staticChallengeEcho = need.captured(3) == "1";
        req.staticChallenge = need.captured(4);
        emit passwordNeeded(req);
        return;
    }

    const QRegularExpressionMatch failed = failedRe.match(payload);
    if (failed.hasMatch())
        emit authFailed(failed.captured(1), failed.captured(2));
}
