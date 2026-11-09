#pragma once
#include <QString>
#include <vector>
#include <QRectF>

struct KeyGeometry {
    int id = 0;
    QString label;
    QString subLabel;
    qreal x = 0.0;
    qreal y = 0.0;
    qreal width = 1.0;
    qreal height = 1.0;
    int row = 0;
    bool isNumpad = false;
    bool isArrow = false;

    QRectF rect() const { return QRectF(x, y, width, height); }
};

class KeyboardLayout {
public:
    KeyboardLayout();

    const std::vector<KeyGeometry>& keys() const { return m_keys; }
    qreal totalWidth() const { return m_totalWidth; }
    qreal totalHeight() const { return m_totalHeight; }

    int keyAt(qreal x, qreal y) const;
    const KeyGeometry* findKey(int id) const;

    static KeyboardLayout createLenovoLoq15Layout();

private:
    std::vector<KeyGeometry> m_keys;
    qreal m_totalWidth = 19.6;
    qreal m_totalHeight = 6.8;
};
