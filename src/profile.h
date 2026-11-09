#pragma once
#include <QColor>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <array>
#include <QMetaType>

enum class Effect { Static, Breathing, Wave, Hue };
enum class Direction { LeftToRight, RightToLeft };
enum class PresetColorMode { Same, Multi, Individual, FixedSections, Single };

struct Profile {
    QString name = "Custom";
    Effect effect = Effect::Static;
    std::array<QColor, 4> zones{QColor("#18d9ff"), QColor("#a43cff"), QColor("#ff3d88"), QColor("#ffb02e")};
    int brightness = 80;
    int speed = 50;
    Direction direction = Direction::LeftToRight;
    bool operator==(const Profile &o) const;
    bool isValid(QString *error = nullptr) const;
    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &json, Profile *out, QString *error = nullptr);
};

QString effectName(Effect effect);
QString directionName(Direction direction);
Effect effectFromName(const QString &name, bool *ok = nullptr);
QList<Profile> builtInProfiles();

Q_DECLARE_METATYPE(Profile)
