// Per-connection window: state, addresses, traffic and log.

#pragma once

#include <QWidget>

class Connection;
class ConnectionManager;
class QLabel;
class QPlainTextEdit;
class QPushButton;

class StatusWindow : public QWidget
{
    Q_OBJECT

  public:
    StatusWindow(ConnectionManager *manager, Connection *connection, QWidget *parent = nullptr);

  signals:
    void connectRequested(Connection *connection);

  private:
    void updateState();
    void updateStats();

    ConnectionManager *manager_;
    Connection *connection_;
    QLabel *state_ = nullptr;
    QLabel *vpnIp_ = nullptr;
    QLabel *server_ = nullptr;
    QLabel *traffic_ = nullptr;
    QLabel *duration_ = nullptr;
    QPlainTextEdit *log_ = nullptr;
    QPushButton *connect_ = nullptr;
    QPushButton *disconnect_ = nullptr;
};

QString formatBytes(quint64 bytes);
