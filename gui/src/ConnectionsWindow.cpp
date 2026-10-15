#include "ConnectionsWindow.h"

#include "Connection.h"
#include "ConnectionManager.h"
#include "Icons.h"
#include "StatusWindow.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {

enum Column
{
    ColName,
    ColState,
    ColAddress,
    ColTraffic,
    ColCount
};

constexpr int ConnectionRole = Qt::UserRole + 1;

} // namespace

ConnectionsWindow::ConnectionsWindow(ConnectionManager *manager, QWidget *parent)
    : QWidget(parent, Qt::Window),
      manager_(manager)
{
    setWindowTitle(tr("Connections"));
    setWindowIcon(Icons::forState(Connection::State::Disconnected));
    resize(720, 360);

    auto *layout = new QVBoxLayout(this);

    tree_ = new QTreeWidget;
    tree_->setColumnCount(ColCount);
    tree_->setHeaderLabels({tr("Profile"), tr("State"), tr("VPN address"), tr("Traffic in / out")});
    tree_->setRootIsDecorated(false);
    tree_->setUniformRowHeights(true);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->header()->setSectionResizeMode(ColName, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(ColState, QHeaderView::ResizeToContents);
    layout->addWidget(tree_, 1);

    auto *buttons = new QHBoxLayout;
    connect_ = new QPushButton(tr("Connect"));
    disconnect_ = new QPushButton(tr("Disconnect"));
    status_ = new QPushButton(tr("Status"));
    auto *import = new QPushButton(tr("Import..."));
    auto *folder = new QPushButton(tr("Open folder"));
    auto *settings = new QPushButton(tr("Settings..."));
    buttons->addWidget(connect_);
    buttons->addWidget(disconnect_);
    buttons->addWidget(status_);
    buttons->addStretch();
    buttons->addWidget(import);
    buttons->addWidget(folder);
    buttons->addWidget(settings);
    layout->addLayout(buttons);

    connect(connect_, &QPushButton::clicked, this, [this]()
            {
        if (Connection *c = selected())
            emit connectRequested(c); });
    connect(disconnect_, &QPushButton::clicked, this, [this]()
            {
        if (Connection *c = selected())
            c->stop(); });
    connect(status_, &QPushButton::clicked, this, [this]()
            {
        if (Connection *c = selected())
            emit statusRequested(c); });
    connect(import, &QPushButton::clicked, this, &ConnectionsWindow::importRequested);
    connect(folder, &QPushButton::clicked, this, [this]()
            { QDesktopServices::openUrl(QUrl::fromLocalFile(manager_->configDir())); });
    connect(settings, &QPushButton::clicked, this, &ConnectionsWindow::settingsRequested);

    connect(tree_, &QTreeWidget::itemSelectionChanged, this, &ConnectionsWindow::updateButtons);
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item)
            {
        auto *c = item->data(ColName, ConnectionRole).value<Connection *>();
        if (!c)
            return;
        if (c->isActive())
            emit statusRequested(c);
        else
            emit connectRequested(c); });
    connect(tree_, &QTreeWidget::customContextMenuRequested, this, &ConnectionsWindow::showContextMenu);

    connect(manager_, &ConnectionManager::profilesChanged, this, &ConnectionsWindow::rebuild);
    connect(manager_, &ConnectionManager::connectionStateChanged, this, [this](Connection *c)
            {
        updateRow(c);
        updateButtons(); });

    // traffic counters
    auto *ticker = new QTimer(this);
    connect(ticker, &QTimer::timeout, this, &ConnectionsWindow::updateRows);
    ticker->start(2000);

    rebuild();
}

void ConnectionsWindow::rebuild()
{
    const Connection *sel = selected();
    tree_->clear();
    for (Connection *c : manager_->connections())
    {
        auto *item = new QTreeWidgetItem(tree_);
        item->setData(ColName, ConnectionRole, QVariant::fromValue(c));
        item->setText(ColName, c->name());
        if (c == sel)
            item->setSelected(true);
        updateRow(c);
    }
    updateButtons();
}

void ConnectionsWindow::updateRow(Connection *c)
{
    QTreeWidgetItem *item = itemFor(c);
    if (!item)
        return;
    item->setIcon(ColName, Icons::forState(c->state()));
    QString state = Connection::stateText(c->state());
    if (c->state() != Connection::State::Connected && !c->stateDetail().isEmpty())
        state += " (" + c->stateDetail() + ')';
    item->setText(ColState, state);
    item->setToolTip(ColState, state);
    item->setText(ColAddress, c->state() == Connection::State::Connected ? c->lastOmiState().localIp : QString());
    item->setText(ColTraffic, c->isActive() ? formatBytes(c->bytesIn()) + " / " + formatBytes(c->bytesOut()) : QString());
}

void ConnectionsWindow::updateRows()
{
    for (Connection *c : manager_->connections())
        if (c->isActive())
            updateRow(c);
}

void ConnectionsWindow::updateButtons()
{
    const Connection *c = selected();
    connect_->setEnabled(c && !c->isActive());
    disconnect_->setEnabled(c && c->isActive() && c->state() != Connection::State::Disconnecting);
    status_->setEnabled(c != nullptr);
}

void ConnectionsWindow::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = tree_->itemAt(pos);
    if (!item)
        return;
    auto *c = item->data(ColName, ConnectionRole).value<Connection *>();
    if (!c)
        return;

    QMenu menu;
    QAction *conn = menu.addAction(tr("Connect"), this, [this, c]()
                                   { emit connectRequested(c); });
    conn->setEnabled(!c->isActive());
    QAction *disc = menu.addAction(tr("Disconnect"), c, &Connection::stop);
    disc->setEnabled(c->isActive());
    menu.addAction(tr("Status"), this, [this, c]()
                   { emit statusRequested(c); });
    menu.addSeparator();
    QAction *autoConn = menu.addAction(tr("Connect on startup"));
    autoConn->setCheckable(true);
    autoConn->setChecked(manager_->isAutoConnect(c->name()));
    connect(autoConn, &QAction::toggled, this, [this, c](bool on)
            { manager_->setAutoConnect(c->name(), on); });
    menu.addAction(tr("Forget saved password"), this, [this, c]()
                   { manager_->setSavedPassword(c->name(), QString()); })
        ->setEnabled(manager_->hasSavedPassword(c->name()));
    menu.exec(tree_->viewport()->mapToGlobal(pos));
}

Connection *ConnectionsWindow::selected() const
{
    const QList<QTreeWidgetItem *> items = tree_->selectedItems();
    if (items.isEmpty())
        return nullptr;
    return items.first()->data(ColName, ConnectionRole).value<Connection *>();
}

QTreeWidgetItem *ConnectionsWindow::itemFor(Connection *c) const
{
    for (int i = 0; i < tree_->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = tree_->topLevelItem(i);
        if (item->data(ColName, ConnectionRole).value<Connection *>() == c)
            return item;
    }
    return nullptr;
}
