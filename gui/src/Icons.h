// State icons, drawn at runtime so the GUI needs no image assets.

#pragma once

#include "Connection.h"

#include <QIcon>

namespace Icons {

QIcon forState(Connection::State state);

// tray icon summarizing all connections
QIcon forConnections(const QList<Connection *> &connections);

} // namespace Icons
