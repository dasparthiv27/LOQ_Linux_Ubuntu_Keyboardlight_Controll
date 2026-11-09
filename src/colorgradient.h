#pragma once

#include <QColor>
#include <QString>
#include <QVector>

struct ColorStop {
    qreal position = 0.0;
    QColor color = Qt::white;
};

class ColorGradient {
public:
    ColorGradient();
    explicit ColorGradient(QVector<ColorStop> stops);

    bool setStops(QVector<ColorStop> stops, QString *error = nullptr);
    const QVector<ColorStop> &stops() const { return m_stops; }
    QColor sample(qreal position) const;
    static QColor interpolate(const QColor &from, const QColor &to, qreal amount);

private:
    QVector<ColorStop> m_stops;
};
