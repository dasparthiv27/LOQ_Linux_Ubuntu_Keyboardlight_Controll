#pragma once
#include "profile.h"
#include "animationpresets.h"
#include <QList>
#include <QString>

class ThemeStore {
public:
    static QString configDirectory();
    static QList<Profile> loadCustom(QString *error=nullptr);
    static bool saveCustom(const QList<Profile>& themes, QString *error=nullptr);
    static bool saveLastApplied(const Profile& profile, QString *error=nullptr);
    static bool loadLastApplied(Profile *profile, QString *error=nullptr);
    static bool writeTheme(const QString& path,const Profile&profile,QString*error=nullptr);
    static bool readTheme(const QString& path,Profile*profile,QString*error=nullptr);
    static QList<AnimationPreset> loadCustomPresets(QString *error=nullptr);
    static bool saveCustomPresets(const QList<AnimationPreset> &presets,QString *error=nullptr);
    static bool writePreset(const QString &path,const AnimationPreset &preset,QString *error=nullptr);
    static bool readPreset(const QString &path,AnimationPreset *preset,QString *error=nullptr);
};
