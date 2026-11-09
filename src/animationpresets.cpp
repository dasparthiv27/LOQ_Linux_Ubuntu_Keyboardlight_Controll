#include "animationpresets.h"

#include <QJsonObject>

namespace {

QString previewName(PreviewEffect effect)
{
    switch (effect) {
    case PreviewEffect::Profile: return "profile";
    case PreviewEffect::Rainbow: return "rainbow";
    case PreviewEffect::Fire: return "fire";
    case PreviewEffect::Ripple:
    case PreviewEffect::WaterRipple: return "ripple";
    case PreviewEffect::ColorTransition: return "transition";
    case PreviewEffect::AudioVisualizer: return "audio";
    case PreviewEffect::SystemMonitor: return "system";
    case PreviewEffect::Breathing: return "breathing";
    case PreviewEffect::Wave: return "wave";
    case PreviewEffect::Aurora: return "aurora";
    case PreviewEffect::ColorPulse: return "pulse";
    case PreviewEffect::AlternatingZones: return "alternating";
    case PreviewEffect::CenterOut: return "center-out";
    case PreviewEffect::PingPong: return "ping-pong";
    case PreviewEffect::KeyGlow: return "key-glow";
    case PreviewEffect::HeatMap: return "heat-map";
    case PreviewEffect::OutsideIn: return "outside-in";
    case PreviewEffect::VerticalWave: return "vertical-wave";
    case PreviewEffect::BidirectionalWave: return "bidirectional-wave";
    case PreviewEffect::AlternatingDirection: return "alternating-direction";
    }
    return {};
}

bool previewFromName(const QString &name, PreviewEffect *effect)
{
    const QList<QPair<QString, PreviewEffect>> choices{
        {"profile", PreviewEffect::Profile}, {"rainbow", PreviewEffect::Rainbow},
        {"fire", PreviewEffect::Fire}, {"ripple", PreviewEffect::WaterRipple},
        {"transition", PreviewEffect::ColorTransition}, {"audio", PreviewEffect::AudioVisualizer},
        {"system", PreviewEffect::SystemMonitor}, {"breathing", PreviewEffect::Breathing},
        {"wave", PreviewEffect::Wave}, {"aurora", PreviewEffect::Aurora},
        {"pulse", PreviewEffect::ColorPulse}, {"alternating", PreviewEffect::AlternatingZones},
        {"center-out", PreviewEffect::CenterOut}, {"ping-pong", PreviewEffect::PingPong},
        {"key-glow", PreviewEffect::KeyGlow}, {"heat-map", PreviewEffect::HeatMap},
        {"outside-in", PreviewEffect::OutsideIn}, {"vertical-wave", PreviewEffect::VerticalWave},
        {"bidirectional-wave", PreviewEffect::BidirectionalWave},
        {"alternating-direction", PreviewEffect::AlternatingDirection}};
    for (const auto &choice : choices) {
        if (choice.first == name) {
            *effect = choice.second;
            return true;
        }
    }
    return false;
}

bool categoryFromName(const QString &name, PresetCategory *category)
{
    const QList<QPair<QString, PresetCategory>> choices{
        {"Static", PresetCategory::Static}, {"Gradient", PresetCategory::Gradient},
        {"Animated", PresetCategory::Animated}, {"Directional", PresetCategory::Directional},
        {"Reactive", PresetCategory::Reactive}};
    for (const auto &choice : choices) {
        if (choice.first == name) {
            *category = choice.second;
            return true;
        }
    }
    return false;
}

AnimationPreset makePreset(const QString &id, const QString &name, const QString &description,
                           Effect effect, PresetColorMode colorMode, Direction direction,
                           int speed, const QStringList &colors)
{
    AnimationPreset preset;
    preset.id = id;
    preset.name = name;
    preset.description = description;
    preset.category = effect == Effect::Static ? PresetCategory::Static : PresetCategory::Animated;
    preset.preview = PreviewEffect::Profile;
    preset.profile.name = name;
    preset.profile.effect = effect;
    preset.profile.direction = direction;
    preset.profile.speed = speed;
    preset.profile.brightness = 80;
    for (int i = 0; i < 4; ++i) preset.profile.zones[static_cast<size_t>(i)] = QColor(colors[i]);
    preset.colorMode = colorMode;
    if (colorMode == PresetColorMode::Same || colorMode == PresetColorMode::Single)
        preset.profile.zones.fill(preset.profile.zones[0]);
    return preset;
}

bool validColorMode(Effect effect, PresetColorMode mode)
{
    if (effect == Effect::Static)
        return mode == PresetColorMode::Same || mode == PresetColorMode::Individual;
    if (effect == Effect::Wave)
        return mode == PresetColorMode::Multi || mode == PresetColorMode::FixedSections;
    if (effect == Effect::Breathing)
        return mode == PresetColorMode::Single || mode == PresetColorMode::Multi ||
               mode == PresetColorMode::Individual;
    return false;
}

} // namespace

QString presetCategoryName(PresetCategory category)
{
    switch (category) {
    case PresetCategory::Static: return "Static";
    case PresetCategory::Gradient: return "Gradient";
    case PresetCategory::Animated: return "Animated";
    case PresetCategory::Directional: return "Directional";
    case PresetCategory::Reactive: return "Reactive";
    }
    return "Other";
}

QString presetColorModeName(PresetColorMode mode)
{
    switch (mode) {
    case PresetColorMode::Same: return "same";
    case PresetColorMode::Multi: return "multi";
    case PresetColorMode::Individual: return "individual";
    case PresetColorMode::FixedSections: return "fixed-sections";
    case PresetColorMode::Single: return "single";
    }
    return {};
}

bool presetColorModeFromName(const QString &name, PresetColorMode *mode)
{
    const QList<QPair<QString, PresetColorMode>> choices{
        {"same", PresetColorMode::Same}, {"multi", PresetColorMode::Multi},
        {"individual", PresetColorMode::Individual},
        {"fixed-sections", PresetColorMode::FixedSections},
        {"single", PresetColorMode::Single}};
    for (const auto &choice : choices) {
        if (choice.first == name) {
            if (mode) *mode = choice.second;
            return true;
        }
    }
    return false;
}

void applyPresetColorMode(AnimationPreset *preset, PresetColorMode mode)
{
    if (!preset) return;
    preset->colorMode = mode;
    if (mode == PresetColorMode::Same || mode == PresetColorMode::Single)
        preset->profile.zones.fill(preset->profile.zones[0]);
}

QJsonObject AnimationPreset::toJson() const
{
    QJsonObject profileJson = profile.toJson();
    profileJson.insert("schemaVersion", 1);
    return {{"schemaVersion", 1}, {"id", id}, {"name", name},
            {"description", description}, {"category", presetCategoryName(category)},
            {"preview", previewName(preview)}, {"colorMode", presetColorModeName(colorMode)},
            {"profile", profileJson}, {"durationSeconds", durationSeconds},
            {"fadeInSeconds", fadeInSeconds}, {"fadeOutSeconds", fadeOutSeconds},
            {"looping", looping}, {"reverse", reverse}};
}

bool AnimationPreset::fromJson(const QJsonObject &json, AnimationPreset *out, QString *error)
{
    auto fail = [error](const QString &message) {
        if (error) *error = message;
        return false;
    };
    if (!out) return fail("Missing animation preset output.");
    if (json.value("schemaVersion").toInt(-1) != 1)
        return fail("Unsupported animation preset schema.");

    AnimationPreset preset;
    preset.id = json.value("id").toString().trimmed();
    preset.name = json.value("name").toString().trimmed();
    preset.description = json.value("description").toString().trimmed();
    if (preset.id.isEmpty() || preset.name.isEmpty() || preset.description.isEmpty())
        return fail("Preset id, name and description are required.");
    if (!categoryFromName(json.value("category").toString(), &preset.category))
        return fail("Unknown animation preset category.");
    if (!previewFromName(json.value("preview").toString(), &preset.preview))
        return fail("Unknown preview animation type.");
    if (!Profile::fromJson(json.value("profile").toObject(), &preset.profile, error))
        return false;

    if (json.contains("colorMode")) {
        if (!presetColorModeFromName(json.value("colorMode").toString(), &preset.colorMode))
            return fail("Unknown preset colour mode.");
    } else if (preset.profile.effect == Effect::Wave) {
        preset.colorMode = PresetColorMode::FixedSections;
    } else if (preset.profile.effect == Effect::Breathing) {
        preset.colorMode = PresetColorMode::Individual;
    } else {
        preset.colorMode = PresetColorMode::Individual;
    }
    if (preset.profile.effect == Effect::Hue) {
        preset.profile.effect = Effect::Wave;
        preset.colorMode = PresetColorMode::Multi;
    }
    if (!validColorMode(preset.profile.effect, preset.colorMode))
        return fail("Colour mode is not supported by this preset effect.");
    applyPresetColorMode(&preset, preset.colorMode);

    preset.durationSeconds = json.value("durationSeconds").toInt(30);
    preset.fadeInSeconds = json.value("fadeInSeconds").toInt(0);
    preset.fadeOutSeconds = json.value("fadeOutSeconds").toInt(0);
    preset.looping = json.value("looping").toBool(true);
    preset.reverse = json.value("reverse").toBool(false);
    if (preset.durationSeconds < 1 || preset.durationSeconds > 600 ||
        preset.fadeInSeconds < 0 || preset.fadeInSeconds > preset.durationSeconds ||
        preset.fadeOutSeconds < 0 || preset.fadeOutSeconds > preset.durationSeconds)
        return fail("Preset duration and fade values are outside supported limits.");
    preset.custom = true;
    *out = preset;
    if (error) error->clear();
    return true;
}

QList<AnimationPreset> builtInAnimationPresets()
{
    const Direction left = Direction::LeftToRight;
    const QStringList rainbow{"#ff3030", "#f5e642", "#24e879", "#2587ff"};
    return {
        makePreset("rainbow-wave", "Rainbow Wave", "Multi-colour wave from left to right.",
                   Effect::Wave, PresetColorMode::Multi, left, 55, rainbow),
        makePreset("reverse-rainbow", "Reverse Rainbow", "Multi-colour wave from right to left.",
                   Effect::Wave, PresetColorMode::Multi, Direction::RightToLeft, 55, rainbow),
        makePreset("ocean-wave", "Ocean Wave", "Fixed ocean colours moving across four zones.",
                   Effect::Wave, PresetColorMode::FixedSections, left, 45,
                   {"#082b85", "#087fc1", "#08c6b6", "#8df5d0"}),
        makePreset("aurora-wave", "Aurora Wave", "Fixed aurora colours moving across four zones.",
                   Effect::Wave, PresetColorMode::FixedSections, left, 35,
                   {"#05c994", "#25d9dd", "#4686ff", "#ab59e8"}),
        makePreset("spectrum-wave", "Spectrum Wave", "A faster multi-colour zone wave.",
                   Effect::Wave, PresetColorMode::Multi, left, 70, rainbow),
        makePreset("colour-breathing", "Colour Breathing", "Single cyan colour with a smooth pulse.",
                   Effect::Breathing, PresetColorMode::Single, left, 40,
                   {"#18d9ff", "#18d9ff", "#18d9ff", "#18d9ff"}),
        makePreset("white-breathing", "White Breathing", "Single white colour with a smooth pulse.",
                   Effect::Breathing, PresetColorMode::Single, left, 35,
                   {"#ffffff", "#ffffff", "#ffffff", "#ffffff"}),
        makePreset("zone-breathing", "Zone Breathing", "Four fixed zone colours pulse together.",
                   Effect::Breathing, PresetColorMode::Individual, left, 45,
                   {"#18d9ff", "#a43cff", "#ff3d88", "#ffb02e"})
    };
}

int nextCustomPresetIndex(const QList<AnimationPreset> &presets, const QString &currentPresetId)
{
    QList<int> customIndexes;
    for (int i = 0; i < presets.size(); ++i)
        if (presets[i].custom) customIndexes.append(i);
    if (customIndexes.isEmpty()) return -1;
    for (int i = 0; i < customIndexes.size(); ++i) {
        if (presets[customIndexes[i]].id == currentPresetId)
            return customIndexes[(i + 1) % customIndexes.size()];
    }
    return customIndexes.first();
}

bool renameCustomPreset(QList<AnimationPreset> &presets, const QString &id,
                        const QString &name, QString *error)
{
    const QString normalized = name.trimmed();
    if (normalized.isEmpty()) {
        if (error) *error = "Preset name cannot be empty.";
        return false;
    }
    for (AnimationPreset &preset : presets) {
        if (preset.id != id) continue;
        if (!preset.custom) {
            if (error) *error = "Built-in presets are read-only.";
            return false;
        }
        preset.name = normalized;
        preset.profile.name = normalized;
        if (error) error->clear();
        return true;
    }
    if (error) *error = "Custom preset was not found.";
    return false;
}

bool duplicatePreset(const QList<AnimationPreset> &presets,
                     QList<AnimationPreset> *customPresets, const QString &sourceId,
                     const QString &newId, const QString &newName, QString *error)
{
    if (!customPresets || newId.trimmed().isEmpty() || newName.trimmed().isEmpty()) {
        if (error) *error = "A new preset ID and name are required.";
        return false;
    }
    for (const AnimationPreset &preset : presets) {
        if (preset.id == newId) {
            if (error) *error = "Preset IDs must be unique.";
            return false;
        }
    }
    for (const AnimationPreset &source : presets) {
        if (source.id != sourceId) continue;
        AnimationPreset copy = source;
        copy.id = newId;
        copy.name = newName.trimmed();
        copy.profile.name = copy.name;
        copy.custom = true;
        customPresets->append(copy);
        if (error) error->clear();
        return true;
    }
    if (error) *error = "Source preset was not found.";
    return false;
}

bool deleteCustomPreset(QList<AnimationPreset> &presets, const QString &id, QString *error)
{
    for (int i = 0; i < presets.size(); ++i) {
        if (presets[i].id != id) continue;
        if (!presets[i].custom) {
            if (error) *error = "Built-in presets are read-only.";
            return false;
        }
        presets.removeAt(i);
        if (error) error->clear();
        return true;
    }
    if (error) *error = "Custom preset was not found.";
    return false;
}
