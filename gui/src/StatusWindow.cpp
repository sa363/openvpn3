#include "StatusWindow.h"

#include "Connection.h"
#include "ConnectionManager.h"
#include "Icons.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

QString formatBytes(quint64 bytes)
{
    static const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 4)
    {
        v /= 1024.0;
        ++u;
    }
    return u == 0 ? QStringLiteral("%1 B").arg(bytes) : QStringLiteral("%1 %2").arg(v, 0, 'f', 1).arg(units[u]);
}

StatusWindow::StatusWindow(ConnectionManager *manager, Connection *connection, QWidget *parent)
    : QWidget(parent, Qt::Window),
      manager_(manager),
      connection_(connection)
{
    setWindowTitle(connection->name());
    resize(640, 480);

    auto *layout = new QVBoxLayout(this);

    auto *form = new QFormLayout;
    state_ = new QLabel;
    vpnIp_ = new QLabel;
    server_ = new QLabel;
    traffic_ = new QLabel;
    duration_ = new QLabel;
    for (QLabel *l : {vpnIp_, server_})
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    form->addRow(tr("State:"), state_);
    form->addRow(tr("VPN address:"), vpnIp_);
    form->addRow(tr("Server:"), server_);
    form->addRow(tr("Traffic:"), traffic_);
    form->addRow(tr("Connected for:"), duration_);
    layout->addLayout(form);

    log_ = new QPlainTextEdit;
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(5000);
    log_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    log_->setLineWrapMode(QPlainTextEdit::NoWrap);
    for (const QString &line : connection->log())
        log_->appendPlainText(line);
    layout->addWidget(log_, 1);

    auto *buttons = new QHBoxLayout;
    connect_ = new QPushButton(tr("Connect"));
    disconnect_ = new QPushButton(tr("Disconnect"));
    auto *copyLog = new QPushButton(tr("Copy log"));
    auto *editConfig = new QPushButton(tr("Edit profile"));
    auto *close = new QPushButton(tr("Hide"));
    buttons->addWidget(connect_);
    buttons->addWidget(disconnect_);
    buttons->addStretch();
    buttons->addWidget(copyLog);
    buttons->addWidget(editConfig);
    buttons->addWidget(close);
    layout->addLayout(buttons);

    connect(connect_, &QPushButton::clicked, this, [this]()
            { emit connectRequested(connection_); });
    connect(disconnect_, &QPushButton::clicked, connection_, &Connection::stop);
    connect(copyLog, &QPushButton::clicked, this, [this]()
            { QGuiApplication::clipboard()->setText(connection_->log().join('\n')); });
    connect(editConfig, &QPushButton::clicked, this, [this]()
            {
        // .ovpn files may be associated with another VPN client, so use an editor
#ifdef Q_OS_WIN
        QProcess::startDetached("notepad.exe", {QDir::toNativeSeparators(connection_->configPath())});
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(connection_->configPath()));
#endif
    });
    connect(close, &QPushButton::clicked, this, &QWidget::hide);

    connect(connection_, &Connection::stateChanged, this, &StatusWindow::updateState);
    connect(connection_, &Connection::statsChanged, this, &StatusWindow::updateStats);
    connect(connection_, &Connection::logAppended, this, [this](const QString &line)
            {
        QScrollBar *sb = log_->verticalScrollBar();
        const bool atEnd = sb->value() == sb->maximum();
        if (connection_->log().size() == 1)
            log_->clear(); // a new session has started
        log_->appendPlainText(line);
        if (atEnd)
            sb->setValue(sb->maximum()); });

    auto *ticker = new QTimer(this);
    connect(ticker, &QTimer::timeout, this, &StatusWindow::updateStats);
    ticker->start(1000);

    updateState();
    updateStats();
}

void StatusWindow::updateState()
{
    const Connection::State st = connection_->state();
    setWindowIcon(Icons::forState(st));

    QString text = Connection::stateText(st);
    if (!connection_->stateDetail().isEmpty() && st != Connection::State::Connected)
        text += " (" + connection_->stateDetail() + ')';
    state_->setText(text);

    const OmiState &omi = connection_->lastOmiState();
    if (st == Connection::State::Connected)
    {
        QStringList ips;
        for (const QString &ip : {omi.localIp, omi.localIp6})
            if (!ip.isEmpty())
                ips << ip;
        vpnIp_->setText(ips.join(", "));
        server_->setText(omi.remotePort.isEmpty() ? omi.remoteIp : omi.remoteIp + ':' + omi.remotePort);
    }
    else
    {
        vpnIp_->clear();
        server_->clear();
    }

    connect_->setEnabled(!connection_->isActive());
    disconnect_->setEnabled(connection_->isActive() && st != Connection::State::Disconnecting);
}

void StatusWindow::updateStats()
{
    traffic_->setText(tr("in %1, out %2").arg(formatBytes(connection_->bytesIn()), formatBytes(connection_->bytesOut())));
    const QDateTime since = connection_->connectedSince();
    if (since.isValid())
    {
        const qint64 s = since.secsTo(QDateTime::currentDateTime());
        duration_->setText(QStringLiteral("%1:%2:%3")
                               .arg(s / 3600)
                               .arg((s / 60) % 60, 2, 10, QChar('0'))
                               .arg(s % 60, 2, 10, QChar('0')));
    }
    else
        duration_->clear();
}
