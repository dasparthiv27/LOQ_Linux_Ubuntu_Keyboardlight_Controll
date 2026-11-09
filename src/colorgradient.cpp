#include "colorgradient.h"

#include <algorithm>

ColorGradient::ColorGradient()
    : m_stops{{0.0, QColor("#18d9ff")}, {1.0, QColor("#ff3d88")}}
{
}

ColorGradient::ColorGradient(QVector<ColorStop> stops)
    : ColorGradient()
{
    setStops(std::move(stops));
}

bool ColorGradient::setStops(QVector<ColorStop> stops, QString *error)
{
    if (stops.size() < 2) {
        if (error) *error = QStringLiteral("A gradient requires at least two colour stops.");
        return false;
    }
    for (const ColorStop &stop : stops) {
        if (!stop.color.isValid() || stop.position < 0.0 || stop.position > 1.0) {
            if (error) *error = QStringLiteral("Gradient stops need valid colours and positions from 0 to 1.");
            return false;
        }
    }
    std::sort(stops.begin(), stops.end(), [](const ColorStop &a, const ColorStop &b) {
        return a.position < b.position;
    });
    if (stops.first().position != 0.0 || stops.last().position != 1.0) {
        if (error) *error = QStringLiteral("Gradient stops must cover both endpoints (0 and 1).");
        return false;
    }
    m_stops = std::move(stops);
    if (error) error->clear();
    return true;
}

QColor ColorGradient::interpolate(const QColor &from, const QColor &to, qreal amount)
{
    const qreal t = qBound(0.0, amount, 1.0);
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * t,
                            from.greenF() + (to.greenF() - from.greenF()) * t,
                            from.blueF() + (to.blueF() - from.blueF()) * t,
                            from.alphaF() + (to.alphaF() - from.alphaF()) * t);
}

QColor ColorGradient::sample(qreal position) const
{
    const qreal t = qBound(0.0, position, 1.0);
    for (qsizetype i = 1; i < m_stops.size(); ++i) {
        if (t <= m_stops[i].position) {
            const ColorStop &left = m_stops[i - 1];
            const ColorStop &right = m_stops[i];
            const qreal span = right.position - left.position;
            const qreal amount = span <= 0.0 ? 1.0 : (t - left.position) / span;
            return interpolate(left.color, right.color, amount);
        }
    }
    return m_stops.last().color;
}
