// Overview of all profiles and their connection state.

#pragma once

#include <QWidget>

class Connection;
class ConnectionManager;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

class ConnectionsWindow : public QWidget
{
    Q_OBJECT

  public:
    explicit ConnectionsWindow(ConnectionManager *manager, QWidget *parent = nullptr);

  signals:
    void connectRequested(Connection *c);
    void statusRequested(Connection *c);
    void importRequested();
    void settingsRequested();

  private:
    void rebuild();
    void updateRow(Connection *c);
    void updateRows();
    void updateButtons();
    void showContextMenu(const QPoint &pos);
    Connection *selected() const;
    QTreeWidgetItem *itemFor(Connection *c) const;

    ConnectionManager *manager_;
    QTreeWidget *tree_ = nullptr;
    QPushButton *connect_ = nullptr;
    QPushButton *disconnect_ = nullptr;
    QPushButton *status_ = nullptr;
};
