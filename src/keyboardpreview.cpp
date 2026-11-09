#include "keyboardpreview.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QLinearGradient>

KeyboardPreview::KeyboardPreview(QWidget *p) : QWidget(p) {
    setMinimumHeight(240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_colors = {QColor("#18d9ff"), QColor("#a43cff"), QColor("#ff3d88"), QColor("#ffb02e")};
    setCursor(Qt::PointingHandCursor);

    m_layout = KeyboardLayout::createLenovoLoq15Layout();
    m_zoneMap = KeyboardZoneMap::createDefaultLoqMap(m_layout);
}

void KeyboardPreview::setColors(const std::array<QColor, 4> &c) {
    m_colors = c;
    update();
}

void KeyboardPreview::setKeyColors(const QHash<int, QColor> &colors) {
    m_keyColors = colors;
    update();
}

void KeyboardPreview::setSelectedZone(int z) {
    m_selected = qBound(0, z, 3);
    update();
}

void KeyboardPreview::setZoneMap(const KeyboardZoneMap &map) {
    m_zoneMap = map;
    update();
}

QRectF KeyboardPreview::getKeyboardBounds() const {
    const qreal padX = 14.0;
    const qreal padY = 12.0;
    const qreal bottomBarHeight = 28.0;

    const qreal availW = width() - padX * 2.0;
    const qreal availH = height() - padY * 2.0 - bottomBarHeight;

    const qreal targetAspect = m_layout.totalWidth() / m_layout.totalHeight();
    const qreal availAspect = availW / (availH > 0 ? availH : 1.0);

    qreal kbW = availW;
    qreal kbH = availH;
    if (availAspect > targetAspect) {
        kbW = availH * targetAspect;
    } else {
        kbH = availW / targetAspect;
    }

    const qreal ox = padX + (availW - kbW) / 2.0;
    const qreal oy = padY + (availH - kbH) / 2.0;

    return QRectF(ox, oy, kbW, kbH);
}

QPointF KeyboardPreview::screenToLayoutPoint(const QPointF &screenPt, const QRectF &bounds) const {
    if (bounds.width() <= 0 || bounds.height() <= 0) return {-1, -1};
    const qreal lx = ((screenPt.x() - bounds.left()) / bounds.width()) * m_layout.totalWidth();
    const qreal ly = ((screenPt.y() - bounds.top()) / bounds.height()) * m_layout.totalHeight();
    return {lx, ly};
}

void KeyboardPreview::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF kbBounds = getKeyboardBounds();

    // 1. Draw outer chassis well
    const QRectF wellRect = kbBounds.adjusted(-10, -10, 10, 10);
    QPainterPath wellPath;
    wellPath.addRoundedRect(wellRect, 12, 12);
    p.fillPath(wellPath, QColor("#0e1219"));
    p.setPen(QPen(QColor("#242d3e"), 1.2));
    p.drawPath(wellPath);

    // Subtle inner bezel
    p.setPen(QPen(QColor("#080a0f"), 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(kbBounds.adjusted(-3, -3, 3, 3), 6, 6);

    const qreal scaleX = kbBounds.width() / m_layout.totalWidth();
    const qreal scaleY = kbBounds.height() / m_layout.totalHeight();

    QFont mainFont("Inter", 7, QFont::DemiBold);
    QFont subFont("Inter", 5, QFont::Normal);

    // 2. Render each key with zone colors, ambient glows, and selection states
    for (const auto &k : m_layout.keys()) {
        const int zone = qBound(0, m_zoneMap.zoneForKey(k.id), 3);
        const QColor zoneColor = m_keyColors.value(k.id, m_colors[static_cast<size_t>(zone)]);
        const bool isSelected = (zone == m_selected);

        const QRectF screenRect(
            kbBounds.left() + k.x * scaleX,
            kbBounds.top() + k.y * scaleY,
            k.width * scaleX,
            k.height * scaleY
        );

        // Under-key backlight glow
        QColor glowColor = zoneColor;
        glowColor.setAlpha(isSelected ? 90 : 45);
        p.setPen(Qt::NoPen);
        p.setBrush(glowColor);
        p.drawRoundedRect(screenRect.adjusted(-1.5, -1.0, 1.5, 1.0), 4.5, 4.5);

        // Keycap body (sculpted chiclet gradient)
        QLinearGradient capGrad(screenRect.topLeft(), screenRect.bottomLeft());
        if (isSelected) {
            capGrad.setColorAt(0.0, QColor("#222b3b"));
            capGrad.setColorAt(1.0, QColor("#161d28"));
        } else {
            capGrad.setColorAt(0.0, QColor("#1a202c"));
            capGrad.setColorAt(1.0, QColor("#12161f"));
        }
        p.setBrush(capGrad);

        // Keycap border: selected zone has bright crisp highlight, unselected has soft zone tint
        if (isSelected) {
            p.setPen(QPen(zoneColor, 1.6));
        } else {
            QColor borderCol = zoneColor;
            borderCol.setAlpha(75);
            p.setPen(QPen(borderCol, 0.8));
        }
        p.drawRoundedRect(screenRect, 3.5, 3.5);

        // Key labels
        p.setFont(mainFont);
        if (isSelected) {
            p.setPen(QColor("#ffffff"));
        } else {
            p.setPen(QColor("#cdd6e5"));
        }

        if (k.subLabel.isEmpty()) {
            p.drawText(screenRect, Qt::AlignCenter, k.label);
        } else {
            // Dual label (number + symbol, or numpad + arrow)
            const QRectF topHalf(screenRect.left(), screenRect.top() + 1, screenRect.width(), screenRect.height() * 0.48);
            const QRectF btmHalf(screenRect.left(), screenRect.top() + screenRect.height() * 0.44, screenRect.width(), screenRect.height() * 0.52);

            p.drawText(topHalf, Qt::AlignCenter, k.label);
            p.setFont(subFont);
            p.setPen(isSelected ? QColor("#a6b4c9") : QColor("#79869a"));
            p.drawText(btmHalf, Qt::AlignCenter, k.subLabel);
        }
    }

    // 3. Draw bottom Zone Indicators Bar
    const qreal barY = height() - 25.0;
    const qreal badgeWidth = (kbBounds.width() - 3.0 * 8.0) / 4.0;

    for (int z = 0; z < 4; ++z) {
        const QRectF badge(kbBounds.left() + z * (badgeWidth + 8.0), barY, badgeWidth, 20.0);
        const bool isSel = (z == m_selected);
        const QColor zCol = m_colors[static_cast<size_t>(z)];

        QColor bg = isSel ? QColor("#1c2535") : QColor("#111620");
        p.setBrush(bg);
        p.setPen(QPen(isSel ? zCol : QColor("#293345"), isSel ? 1.4 : 0.8));
        p.drawRoundedRect(badge, 5, 5);

        // Color dot
        p.setBrush(zCol);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(badge.left() + 10.0, badge.center().y()), 4.0, 4.0);

        // Text
        p.setFont(QFont("Inter", 7, isSel ? QFont::Bold : QFont::Normal));
        p.setPen(isSel ? QColor("#ffffff") : QColor("#8f9cb3"));
        p.drawText(QRectF(badge.left() + 20.0, badge.top(), badge.width() - 24.0, badge.height()),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   KeyboardZoneMap::zoneName(z));
    }
}

void KeyboardPreview::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        const QRectF kbBounds = getKeyboardBounds();
        const QPointF pos = e->position();

        // 1. Check if clicked on the bottom zone selector bar
        const qreal barY = height() - 25.0;
        if (pos.y() >= barY && pos.y() <= barY + 22.0) {
            const qreal badgeWidth = (kbBounds.width() - 3.0 * 8.0) / 4.0;
            for (int z = 0; z < 4; ++z) {
                const QRectF badge(kbBounds.left() + z * (badgeWidth + 8.0), barY, badgeWidth, 20.0);
                if (badge.contains(pos)) {
                    setSelectedZone(z);
                    emit zoneClicked(z);
                    return;
                }
            }
        }

        // 2. Check if clicked on an actual key
        if (kbBounds.contains(pos)) {
            const QPointF layoutPt = screenToLayoutPoint(pos, kbBounds);
            const int keyId = m_layout.keyAt(layoutPt.x(), layoutPt.y());
            if (keyId > 0) {
                const int zone = m_zoneMap.zoneForKey(keyId);
                const KeyGeometry *key = m_layout.findKey(keyId);
                if (key) {
                    emit keyActivated(keyId,
                                      QPointF(key->x + key->width / 2.0,
                                              key->y + key->height / 2.0),
                                      zone);
                }
                setSelectedZone(zone);
                emit zoneClicked(zone);
                return;
            }
        }

        // 3. Fallback: closest zone horizontally
        const int fallbackZone = qBound(0, int(((pos.x() - kbBounds.left()) / (kbBounds.width() > 0 ? kbBounds.width() : 1.0)) * 4.0), 3);
        setSelectedZone(fallbackZone);
        emit zoneClicked(fallbackZone);
    }
    QWidget::mousePressEvent(e);
}
