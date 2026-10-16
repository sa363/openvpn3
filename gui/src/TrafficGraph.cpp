#include "TrafficGraph.h"

#include "Connection.h"

#include <QDateTime>
#include <QPainter>
#include <QPainterPath>

#include <cmath>

namespace {

// the graph always shows this time span, newest sample at the right edge
constexpr qint64 spanMs = 5 * 60 * 1000;

const QColor inColor(0x2e, 0xa0, 0x43);
const QColor outColor(0x3b, 0x82, 0xf6);

// round up to 1, 2 or 5 times a power of ten, so the scale label reads well
double niceCeiling(double v)
{
    if (v <= 0)
        return 10000;
    const double p = std::pow(10.0, std::floor(std::log10(v)));
    for (const double m : {1.0, 2.0, 5.0, 10.0})
        if (v <= m * p)
            return m * p;
    return 10 * p;
}

} // namespace

TrafficGraph::TrafficGraph(Connection *connection, QWidget *parent)
    : QWidget(parent),
      connection_(connection)
{
    setMinimumHeight(90);
    connect(connection_, &Connection::statsChanged, this, qOverload<>(&QWidget::update));
    connect(connection_, &Connection::stateChanged, this, qOverload<>(&QWidget::update));
}

QSize TrafficGraph::sizeHint() const
{
    return QSize(400, 120);
}

QString TrafficGraph::formatRate(double bytesPerSecond)
{
    // bits per second, as usual for network speeds
    double v = bytesPerSecond * 8;
    static const char *units[] = {"bit/s", "kbit/s", "Mbit/s", "Gbit/s"};
    int u = 0;
    while (v >= 1000 && u < 3)
    {
        v /= 1000;
        ++u;
    }
    return QStringLiteral("%1 %2").arg(v, 0, 'f', u == 0 || v >= 100 ? 0 : 1).arg(units[u]);
}

void TrafficGraph::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPalette &pal = palette();
    const QRectF area = QRectF(rect()).adjusted(1, 18, -1, -1);
    p.fillRect(area, pal.color(QPalette::Base));
    p.setPen(pal.color(QPalette::Mid));
    p.drawRect(area);

    const QList<TrafficSample> &samples = connection_->trafficHistory();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    double peak = 0;
    for (const TrafficSample &s : samples)
        peak = qMax(peak, qMax(s.in, s.out));
    // rounded in bits, as the labels show bits per second
    const double scale = niceCeiling(peak * 8) / 8;

    // grid: quarters of the scale
    p.setPen(QPen(pal.color(QPalette::Midlight), 1, Qt::DotLine));
    for (int i = 1; i < 4; ++i)
    {
        const qreal y = area.bottom() - area.height() * i / 4;
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }

    auto point = [&](qint64 time, double value)
    {
        const qreal x = area.right() - area.width() * double(now - time) / spanMs;
        const qreal y = area.bottom() - area.height() * qMin(value / scale, 1.0);
        return QPointF(qMax(x, area.left()), y);
    };

    auto drawSeries = [&](const QColor &color, double TrafficSample::*field)
    {
        QPainterPath path;
        bool first = true;
        for (const TrafficSample &s : samples)
        {
            if (now - s.time > spanMs)
                continue;
            const QPointF pt = point(s.time, s.*field);
            if (first)
                path.moveTo(pt);
            else
                path.lineTo(pt);
            first = false;
        }
        p.setPen(QPen(color, 1.5));
        p.drawPath(path);
    };

    p.save();
    p.setClipRect(area);
    drawSeries(inColor, &TrafficSample::in);
    drawSeries(outColor, &TrafficSample::out);
    p.restore();

    // legend with the current rates, and the scale
    const double curIn = samples.isEmpty() ? 0 : samples.last().in;
    const double curOut = samples.isEmpty() ? 0 : samples.last().out;
    const QFontMetrics fm(font());
    qreal x = 2;
    auto legend = [&](const QColor &color, const QString &text)
    {
        p.fillRect(QRectF(x, 5, 10, 3), color);
        x += 14;
        p.setPen(pal.color(QPalette::WindowText));
        p.drawText(QPointF(x, 12), text);
        x += fm.horizontalAdvance(text) + 16;
    };
    legend(inColor, tr("in %1").arg(formatRate(curIn)));
    legend(outColor, tr("out %1").arg(formatRate(curOut)));

    const QString scaleText = tr("%1, last 5 min").arg(formatRate(scale));
    p.setPen(pal.color(QPalette::PlaceholderText));
    p.drawText(QPointF(width() - fm.horizontalAdvance(scaleText) - 2, 12), scaleText);
}
