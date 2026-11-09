#include "keyboardzonemap.h"
#include <algorithm>

KeyboardZoneMap::KeyboardZoneMap() = default;

QString KeyboardZoneMap::zoneName(int zone) {
    switch (zone) {
    case Zone1_Left: return "Zone 1 · Left";
    case Zone2_CenterLeft: return "Zone 2 · Center-Left";
    case Zone3_CenterRight: return "Zone 3 · Center-Right";
    case Zone4_Numpad: return "Zone 4 · Numpad";
    default: return QString("Zone %1").arg(zone + 1);
    }
}

int KeyboardZoneMap::zoneForKey(int keyId) const {
    auto it = m_keyToZone.find(keyId);
    return it != m_keyToZone.end() ? it->second : -1;
}

void KeyboardZoneMap::setKeyZone(int keyId, int zone) {
    if (isValidZone(zone)) {
        m_keyToZone[keyId] = zone;
    }
}

QList<int> KeyboardZoneMap::keysInZone(int zone) const {
    QList<int> res;
    for (const auto &pair : m_keyToZone) {
        if (pair.second == zone) {
            res.append(pair.first);
        }
    }
    std::sort(res.begin(), res.end());
    return res;
}

KeyboardZoneMap KeyboardZoneMap::createDefaultLoqMap(const KeyboardLayout &layout) {
    KeyboardZoneMap map;

    // Marked as estimated based on Lenovo Vantage 4-zone layout specification
    map.m_verified = false;
    map.m_provenance = "Estimated based on Lenovo Vantage 4-zone spectrum layout (Zone 1: Left QWERTY, "
                       "Zone 2: Center-Left, Zone 3: Center-Right & Arrows, Zone 4: Numpad). "
                       "Unverified against physical hardware report descriptors - "
                       "please provide a Lenovo Vantage screenshot if physical grouping discrepancies exist.";

    for (const auto &k : layout.keys()) {
        int z = 0;

        if (k.isNumpad) {
            z = Zone4_Numpad;
        } else if (k.isArrow) {
            z = Zone3_CenterRight;
        } else {
            // Main typing block partitioned by physical column position
            switch (k.row) {
            case 0: // Function row
                if (k.label == "Esc" || k.label == "F1" || k.label == "F2" || k.label == "F3" || k.label == "F4") {
                    z = Zone1_Left;
                } else if (k.label == "F5" || k.label == "F6" || k.label == "F7" || k.label == "F8") {
                    z = Zone2_CenterLeft;
                } else {
                    z = Zone3_CenterRight; // F9-F12, PrtSc, Insert, Delete
                }
                break;

            case 1: // Number row
                if (k.label == "~" || k.label == "1" || k.label == "2" || k.label == "3" || k.label == "4") {
                    z = Zone1_Left;
                } else if (k.label == "5" || k.label == "6" || k.label == "7" || k.label == "8") {
                    z = Zone2_CenterLeft;
                } else {
                    z = Zone3_CenterRight; // 9, 0, -, =, Backspace
                }
                break;

            case 2: // QWERTY row
                if (k.label == "Tab" || k.label == "Q" || k.label == "W" || k.label == "E" || k.label == "R") {
                    z = Zone1_Left;
                } else if (k.label == "T" || k.label == "Y" || k.label == "U") {
                    z = Zone2_CenterLeft;
                } else {
                    z = Zone3_CenterRight; // I, O, P, [, ], Backslash
                }
                break;

            case 3: // Home row
                if (k.label == "Caps" || k.label == "A" || k.label == "S" || k.label == "D" || k.label == "F") {
                    z = Zone1_Left;
                } else if (k.label == "G" || k.label == "H" || k.label == "J") {
                    z = Zone2_CenterLeft;
                } else {
                    z = Zone3_CenterRight; // K, L, ;, ', Enter
                }
                break;

            case 4: // Shift row
                if (k.label == "LShift") {
                    z = Zone1_Left;
                } else if (k.label == "RShift") {
                    z = Zone3_CenterRight;
                } else if (k.label == "Z" || k.label == "X" || k.label == "C" || k.label == "V") {
                    z = Zone1_Left;
                } else if (k.label == "B" || k.label == "N" || k.label == "M") {
                    z = Zone2_CenterLeft;
                } else {
                    z = Zone3_CenterRight; // ,, ., /, RShift
                }
                break;

            case 5: // Bottom row
                if (k.label == "Ctrl" && k.x < 2.0) {
                    z = Zone1_Left;
                } else if (k.label == "Fn" || k.label == "Win" || (k.label == "Alt" && k.x < 4.0)) {
                    z = Zone1_Left;
                } else if (k.label == "Space") {
                    z = Zone2_CenterLeft;
                } else {
                    z = Zone3_CenterRight; // RAlt, RCtrl
                }
                break;

            default:
                // Fallback geometry partition
                if (k.x < 5.0) z = Zone1_Left;
                else if (k.x < 10.0) z = Zone2_CenterLeft;
                else z = Zone3_CenterRight;
                break;
            }
        }

        map.setKeyZone(k.id, z);
    }

    return map;
}
