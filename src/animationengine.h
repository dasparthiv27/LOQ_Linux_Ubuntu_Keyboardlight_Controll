#pragma once
#include "colorgradient.h"
#include "keyboardlayout.h"
#include "keyboardzonemap.h"
#include "profile.h"
#include <QHash>
#include <QObject>
#include <QPointF>
#include <optional>
#include <QVector>

enum class PreviewEffect {
    Profile, Rainbow, Fire, Ripple, ColorTransition, AudioVisualizer, SystemMonitor,
    Breathing, Wave, Aurora, ColorPulse, AlternatingZones, CenterOut, PingPong,
    WaterRipple, KeyGlow, HeatMap, OutsideIn, VerticalWave, BidirectionalWave,
    AlternatingDirection
};

class AnimationEngine final : public QObject {
    Q_OBJECT
public:
    explicit AnimationEngine(QObject *parent=nullptr);
    void setProfile(const Profile &profile);
    void setPreviewEffect(PreviewEffect effect);
    void setPresetColorMode(PresetColorMode mode);
    void setAudioLevel(std::optional<qreal> level);
    void setSystemLoad(std::optional<qreal> load);
    void setPalette(const ColorGradient &palette);
    void setRainbowSaturation(qreal saturation);
    void setCustomRainbowPalette(bool enabled);
    void setBreathingDepth(qreal depth);
    void setReactiveColor(const QColor &color);
    void setReactiveDecay(qreal seconds);
    void setRippleSettings(qreal speed, qreal maxRadius, int waveCount, qreal intensity);
    void setPlayback(qreal durationSeconds, qreal fadeInSeconds,
                     qreal fadeOutSeconds, bool looping, bool reverse);
    void registerKeyPress(int keyId, const QPointF &position);
    void clearReactiveState();
    void setPaused(bool paused) { m_paused = paused; }
    bool isPaused() const { return m_paused; }
    qreal phase() const { return m_phase; }
    qreal elapsed() const { return m_elapsed; }
    qreal duration() const { return m_duration; }
    const Profile &profile() const { return m_profile; }
    std::array<QColor,4> frame() const;
    QHash<int, QColor> keyFrame(const KeyboardLayout &layout,
                                const KeyboardZoneMap &zoneMap) const;
    void advance(qreal seconds);
signals:
    void frameChanged();
private:
    Profile m_profile;
    qreal m_phase=0.0;
    PreviewEffect m_previewEffect=PreviewEffect::Profile;
    PresetColorMode m_presetColorMode=PresetColorMode::Individual;
    std::optional<qreal> m_audioLevel;
    std::optional<qreal> m_systemLoad;
    struct Ripple { QPointF position; qreal age = 0.0; };
    QHash<int, qreal> m_keyHeat;
    QVector<Ripple> m_ripples;
    ColorGradient m_palette;
    QColor m_reactiveColor = QColor("#18d9ff");
    qreal m_decaySeconds = 2.0;
    bool m_paused = false;
    bool m_looping = true;
    bool m_reverse = false;
    bool m_customRainbowPalette = false;
    qreal m_rainbowSaturation = 0.94;
    qreal m_breathingDepth = 0.84;
    qreal m_rippleSpeed = 4.0;
    qreal m_rippleMaxRadius = 12.0;
    qreal m_reactiveIntensity = 1.0;
    int m_waveCount = 1;
    qreal m_elapsed = 0.0;
    qreal m_duration = 0.0;
    qreal m_fadeInSeconds = 0.0;
    qreal m_fadeOutSeconds = 0.0;
};
