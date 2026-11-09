#include "keyboardlayout.h"

KeyboardLayout::KeyboardLayout() = default;

int KeyboardLayout::keyAt(qreal x, qreal y) const {
    for (const auto &k : m_keys) {
        if (x >= k.x && x <= (k.x + k.width) && y >= k.y && y <= (k.y + k.height)) {
            return k.id;
        }
    }
    return -1;
}

const KeyGeometry* KeyboardLayout::findKey(int id) const {
    for (const auto &k : m_keys) {
        if (k.id == id) return &k;
    }
    return nullptr;
}

KeyboardLayout KeyboardLayout::createLenovoLoq15Layout() {
    KeyboardLayout layout;
    int nextId = 1;

    auto addKey = [&](const QString &lbl, const QString &sub, qreal x, qreal y, qreal w, qreal h, int row, bool numpad = false, bool arrow = false) {
        KeyGeometry k;
        k.id = nextId++;
        k.label = lbl;
        k.subLabel = sub;
        k.x = x;
        k.y = y;
        k.width = w;
        k.height = h;
        k.row = row;
        k.isNumpad = numpad;
        k.isArrow = arrow;
        layout.m_keys.push_back(k);
    };

    const qreal gap = 0.08;
    const qreal fH = 0.75;
    const qreal normH = 1.0;
    const qreal numpadStartX = 16.25;

    // --- ROW 0: Function row (y = 0.0) ---
    qreal x = 0.0;
    addKey("Esc", "", x, 0.0, 1.0, fH, 0); x += 1.0 + 0.25;
    for (const auto &f : {"F1", "F2", "F3", "F4"}) {
        addKey(f, "", x, 0.0, 0.85, fH, 0); x += 0.85 + gap;
    }
    x += 0.20;
    for (const auto &f : {"F5", "F6", "F7", "F8"}) {
        addKey(f, "", x, 0.0, 0.85, fH, 0); x += 0.85 + gap;
    }
    x += 0.20;
    for (const auto &f : {"F9", "F10", "F11", "F12"}) {
        addKey(f, "", x, 0.0, 0.85, fH, 0); x += 0.85 + gap;
    }
    x += 0.20;
    addKey("Insert", "", x, 0.0, 0.85, fH, 0); x += 0.85 + gap;
    addKey("Delete", "", x, 0.0, 0.85, fH, 0); x += 0.85 + gap;

    qreal nx = numpadStartX;
    for (const auto &nk : {"Num", "/", "*", "-"}) {
        addKey(nk, "", nx, 0.0, 0.88, fH, 0, true);
        nx += 0.88 + gap;
    }

    // --- ROW 1: Numbers (y = 0.95) ---
    qreal y = 0.95;
    x = 0.0;
    addKey("~", "`", x, y, 1.0, normH, 1); x += 1.0 + gap;
    const QString numKeys[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="};
    const QString numSubs[] = {"!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+"};
    for (int i = 0; i < 12; ++i) {
        addKey(numKeys[i], numSubs[i], x, y, 1.0, normH, 1);
        x += 1.0 + gap;
    }
    addKey("Backspace", "", x, y, 1.88, normH, 1);

    nx = numpadStartX;
    for (const auto &nk : {"7", "8", "9", "+"}) {
        addKey(nk, "", nx, y, 0.88, normH, 1, true);
        nx += 0.88 + gap;
    }

    // --- ROW 2: QWERTY (y = 2.07) ---
    y += normH + 0.12;
    x = 0.0;
    addKey("Tab", "", x, y, 1.45, normH, 2); x += 1.45 + gap;
    const QString qKeys[] = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]"};
    for (const auto &q : qKeys) {
        addKey(q, "", x, y, 1.0, normH, 2);
        x += 1.0 + gap;
    }
    addKey("\\", "|", x, y, 1.43, normH, 2);

    nx = numpadStartX;
    for (const auto &nk : {"4", "5", "6", "+"}) {
        addKey(nk, "", nx, y, 0.88, normH, 2, true);
        nx += 0.88 + gap;
    }

    // --- ROW 3: Home Row (y = 3.19) ---
    y += normH + 0.12;
    x = 0.0;
    addKey("Caps", "", x, y, 1.75, normH, 3); x += 1.75 + gap;
    const QString hKeys[] = {"A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'"};
    for (const auto &h : hKeys) {
        addKey(h, "", x, y, 1.0, normH, 3);
        x += 1.0 + gap;
    }
    addKey("Enter", "", x, y, 2.13, normH, 3);

    nx = numpadStartX;
    for (const auto &nk : {"1", "2", "3", "Ent"}) {
        addKey(nk, "", nx, y, 0.88, normH, 3, true);
        nx += 0.88 + gap;
    }

    // --- ROW 4: Shift Row (y = 4.31) ---
    y += normH + 0.12;
    x = 0.0;
    addKey("LShift", "", x, y, 2.25, normH, 4); x += 2.25 + gap;
    const QString zKeys[] = {"Z", "X", "C", "V", "B", "N", "M", ",", ".", "/"};
    for (const auto &z : zKeys) {
        addKey(z, "", x, y, 1.0, normH, 4);
        x += 1.0 + gap;
    }
    addKey("RShift", "", x, y, 1.25, normH, 4); x += 1.25 + gap;

    // Up arrow dropped into inverted-T
    addKey("▲", "", 14.75, y + 0.18, 0.95, 0.85, 4, false, true);

    nx = numpadStartX;
    addKey("0", "Ins", nx, y, 1.84, normH, 4, true); nx += 1.84 + gap;
    addKey(".", "Del", nx, y, 0.88, normH, 4, true); nx += 0.88 + gap;
    addKey("Ent", "", nx, y, 0.88, normH, 4, true);

    // --- ROW 5: Bottom Modifier Row (y = 5.43) ---
    y += normH + 0.12;
    x = 0.0;
    addKey("Ctrl", "", x, y, 1.25, normH, 5); x += 1.25 + gap;
    addKey("Fn", "", x, y, 1.0, normH, 5); x += 1.0 + gap;
    addKey("Win", "", x, y, 1.25, normH, 5); x += 1.25 + gap;
    addKey("Alt", "", x, y, 1.25, normH, 5); x += 1.25 + gap;
    addKey("Space", "", x, y, 5.25, normH, 5); x += 5.25 + gap;
    addKey("Alt", "", x, y, 1.20, normH, 5); x += 1.20 + gap;
    addKey("PrtSc", "", x, y, 1.00, normH, 5); x += 1.00 + gap;

    // Arrow cluster bottom: Left, Down, Right
    const qreal arrowY = y + 0.18;
    qreal ax = 13.67;
    addKey("◄", "", ax, arrowY, 0.95, 0.85, 5, false, true); ax += 0.95 + gap;
    addKey("▼", "", ax, arrowY, 0.95, 0.85, 5, false, true); ax += 0.95 + gap;
    addKey("►", "", ax, arrowY, 0.95, 0.85, 5, false, true);

    // Compute bounding dimensions
    qreal maxX = 0.0, maxY = 0.0;
    for (const auto &k : layout.m_keys) {
        if (k.x + k.width > maxX) maxX = k.x + k.width;
        if (k.y + k.height > maxY) maxY = k.y + k.height;
    }
    layout.m_totalWidth = maxX;
    layout.m_totalHeight = maxY;

    return layout;
}
