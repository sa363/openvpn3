#include "Icons.h"

#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace {

QColor stateColor(Connection::State state)
{
    switch (state)
    {
    case Connection::State::Connected:
        return QColor(0x2e, 0xa0, 0x43);
    case Connection::State::Connecting:
    case Connection::State::Reconnecting:
    case Connection::State::Disconnecting:
        return QColor(0xe0, 0xa1, 0x00);
    case Connection::State::Failed:
        return QColor(0xd1, 0x24, 0x2f);
    case Connection::State::Disconnected:
        break;
    }
    return QColor(0x8c, 0x95, 0x9f);
}

QIcon makeIcon(const QColor &color)
{
    QIcon icon;
    for (const int size : {16, 24, 32, 48, 64})
    {
        QPixmap pm(size, size);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);

        // shield shape
        const qreal s = size;
        QPainterPath path;
        path.moveTo(s * 0.5, s * 0.06);
        path.lineTo(s * 0.88, s * 0.2);
        path.cubicTo(s * 0.88, s * 0.62, s * 0.7, s * 0.82, s * 0.5, s * 0.95);
        path.cubicTo(s * 0.3, s * 0.82, s * 0.12, s * 0.62, s * 0.12, s * 0.2);
        path.closeSubpath();
        p.setPen(QPen(color.darker(140), qMax<qreal>(1.0, s / 24)));
        p.setBrush(color);
        p.drawPath(path);

        // keyhole
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawEllipse(QPointF(s * 0.5, s * 0.42), s * 0.1, s * 0.1);
        p.drawRect(QRectF(s * 0.46, s * 0.45, s * 0.08, s * 0.22));
        p.end();
        icon.addPixmap(pm);
    }
    return icon;
}

} // namespace

namespace Icons {

QIcon forState(Connection::State state)
{
    static QHash<int, QIcon> cache;
    const int key = static_cast<int>(state);
    auto it = cache.find(key);
    if (it == cache.end())
        it = cache.insert(key, makeIcon(stateColor(state)));
    return *it;
}

QIcon forConnections(const QList<Connection *> &connections)
{
    // connected wins over connecting, which wins over failed
    bool connected = false, busy = false, failed = false;
    for (const Connection *c : connections)
    {
        switch (c->state())
        {
        case Connection::State::Connected:
            connected = true;
            break;
        case Connection::State::Connecting:
        case Connection::State::Reconnecting:
        case Connection::State::Disconnecting:
            busy = true;
            break;
        case Connection::State::Failed:
            failed = true;
            break;
        case Connection::State::Disconnected:
            break;
        }
    }
    if (busy)
        return forState(Connection::State::Connecting);
    if (connected)
        return forState(Connection::State::Connected);
    if (failed)
        return forState(Connection::State::Failed);
    return forState(Connection::State::Disconnected);
}

} // namespace Icons
