#pragma once

#include "animationengine.h"
#include "profile.h"
#include <QList>
#include <QJsonObject>
#include <QString>

enum class PresetCategory { Static, Gradient, Animated, Directional, Reactive };

struct AnimationPreset {
    QString id;
    QString name;
    QString description;
    PresetCategory category = PresetCategory::Static;
    Profile profile;
    PreviewEffect preview = PreviewEffect::Profile;
    PresetColorMode colorMode = PresetColorMode::Individual;
    bool nativeControllerCandidate = false;
    bool custom = false;
    int durationSeconds = 30;
    int fadeInSeconds = 0;
    int fadeOutSeconds = 0;
    bool looping = true;
    bool reverse = false;

    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &json, AnimationPreset *out,
                         QString *error = nullptr);
};

QString presetCategoryName(PresetCategory category);
QList<AnimationPreset> builtInAnimationPresets();
QString presetColorModeName(PresetColorMode mode);
bool presetColorModeFromName(const QString &name, PresetColorMode *mode);
void applyPresetColorMode(AnimationPreset *preset, PresetColorMode mode);
int nextCustomPresetIndex(const QList<AnimationPreset> &presets,
                          const QString &currentPresetId);
bool renameCustomPreset(QList<AnimationPreset> &presets, const QString &id,
                        const QString &name, QString *error = nullptr);
bool duplicatePreset(const QList<AnimationPreset> &presets,
                    QList<AnimationPreset> *customPresets,
                    const QString &sourceId, const QString &newId,
                    const QString &newName, QString *error = nullptr);
bool deleteCustomPreset(QList<AnimationPreset> &presets, const QString &id,
                        QString *error = nullptr);
