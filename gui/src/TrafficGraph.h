// Incoming and outgoing traffic rate of a connection over the last minutes.

#pragma once

#include <QWidget>

class Connection;

class TrafficGraph : public QWidget
{
    Q_OBJECT

  public:
    explicit TrafficGraph(Connection *connection, QWidget *parent = nullptr);

    QSize sizeHint() const override;

    static QString formatRate(double bytesPerSecond);

  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    Connection *connection_;
};
