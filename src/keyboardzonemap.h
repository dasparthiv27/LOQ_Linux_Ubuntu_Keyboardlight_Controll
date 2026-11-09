#pragma once
#include "keyboardlayout.h"
#include <unordered_map>
#include <QList>
#include <QString>

class KeyboardZoneMap {
public:
    enum Zone {
        Zone1_Left = 0,
        Zone2_CenterLeft = 1,
        Zone3_CenterRight = 2,
        Zone4_Numpad = 3,
        ZoneCount = 4
    };

    KeyboardZoneMap();

    static KeyboardZoneMap createDefaultLoqMap(const KeyboardLayout &layout);

    int zoneForKey(int keyId) const;
    void setKeyZone(int keyId, int zone);

    QList<int> keysInZone(int zone) const;
    static bool isValidZone(int zone) { return zone >= 0 && zone < ZoneCount; }
    static QString zoneName(int zone);

    bool isVerified() const { return m_verified; }
    QString provenance() const { return m_provenance; }
    void setVerified(bool v, const QString &provenance = {}) {
        m_verified = v;
        m_provenance = provenance;
    }

private:
    std::unordered_map<int, int> m_keyToZone;
    bool m_verified = false;
    QString m_provenance;
};
