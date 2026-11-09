#pragma once
#include "keyboardlayout.h"
#include "keyboardzonemap.h"
#include <QWidget>
#include <QHash>
#include <array>
#include <QColor>

class KeyboardPreview final : public QWidget {
    Q_OBJECT
public:
    explicit KeyboardPreview(QWidget *parent = nullptr);

    void setColors(const std::array<QColor, 4> &colors);
    void setKeyColors(const QHash<int, QColor> &colors);
    void setSelectedZone(int zone);
    int selectedZone() const { return m_selected; }

    const KeyboardLayout& layout() const { return m_layout; }
    const KeyboardZoneMap& zoneMap() const { return m_zoneMap; }
    void setZoneMap(const KeyboardZoneMap &map);

signals:
    void zoneClicked(int zone);
    void keyActivated(int keyId, const QPointF &position, int zone);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    KeyboardLayout m_layout;
    KeyboardZoneMap m_zoneMap;
    std::array<QColor, 4> m_colors;
    QHash<int, QColor> m_keyColors;
    int m_selected = 0;

    QRectF getKeyboardBounds() const;
    QPointF screenToLayoutPoint(const QPointF &screenPt, const QRectF &bounds) const;
};
