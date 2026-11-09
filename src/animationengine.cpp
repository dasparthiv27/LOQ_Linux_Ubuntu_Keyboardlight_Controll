#include "animationengine.h"

#include <QLineF>
#include <QtMath>

namespace {

qreal unitPhase(qreal value)
{
    return value - qFloor(value);
}

QColor scaleColor(const QColor &color, qreal amount)
{
    const qreal level = qBound(0.0, amount, 1.0);
    return QColor::fromRgbF(color.redF() * level, color.greenF() * level,
                            color.blueF() * level, color.alphaF());
}

QColor hueColor(qreal hue, qreal saturation, qreal value)
{
    return QColor::fromHsvF(unitPhase(hue), qBound(0.0, saturation, 1.0),
                            qBound(0.0, value, 1.0));
}

} // namespace

AnimationEngine::AnimationEngine(QObject *p) : QObject(p) {}

void AnimationEngine::setProfile(const Profile &p)
{
    m_profile = p;
    m_phase = 0.0;
    m_elapsed = 0.0;
    m_paused = false;
    emit frameChanged();
}

void AnimationEngine::setPreviewEffect(PreviewEffect e)
{
    m_previewEffect = e;
    emit frameChanged();
}

void AnimationEngine::setPresetColorMode(PresetColorMode mode)
{
    m_presetColorMode = mode;
    emit frameChanged();
}

void AnimationEngine::setAudioLevel(std::optional<qreal> level)
{
    m_audioLevel = level;
    emit frameChanged();
}

void AnimationEngine::setSystemLoad(std::optional<qreal> load)
{
    m_systemLoad = load;
    emit frameChanged();
}

void AnimationEngine::setPalette(const ColorGradient &palette)
{
    m_palette = palette;
    emit frameChanged();
}

void AnimationEngine::setRainbowSaturation(qreal saturation)
{
    m_rainbowSaturation = qBound(0.0, saturation, 1.0);
    emit frameChanged();
}

void AnimationEngine::setCustomRainbowPalette(bool enabled)
{
    m_customRainbowPalette = enabled;
    emit frameChanged();
}

void AnimationEngine::setBreathingDepth(qreal depth)
{
    m_breathingDepth = qBound(0.0, depth, 1.0);
    emit frameChanged();
}

void AnimationEngine::setReactiveColor(const QColor &color)
{
    if (color.isValid()) {
        m_reactiveColor = color;
        emit frameChanged();
    }
}

void AnimationEngine::setReactiveDecay(qreal seconds)
{
    m_decaySeconds = qBound(0.1, seconds, 30.0);
}

void AnimationEngine::setRippleSettings(qreal speed, qreal maxRadius, int waveCount,
                                        qreal intensity)
{
    m_rippleSpeed = qBound(0.2, speed, 30.0);
    m_rippleMaxRadius = qBound(1.0, maxRadius, 40.0);
    m_waveCount = qBound(1, waveCount, 8);
    m_reactiveIntensity = qBound(0.0, intensity, 1.0);
}

void AnimationEngine::setPlayback(qreal durationSeconds, qreal fadeInSeconds,
                                  qreal fadeOutSeconds, bool looping, bool reverse)
{
    m_duration = qMax(0.0, durationSeconds);
    m_fadeInSeconds = qMax(0.0, fadeInSeconds);
    m_fadeOutSeconds = qMax(0.0, fadeOutSeconds);
    m_looping = looping;
    m_reverse = reverse;
    m_elapsed = 0.0;
    m_paused = false;
}

void AnimationEngine::registerKeyPress(int keyId, const QPointF &position)
{
    if (keyId <= 0) return;
    m_keyHeat[keyId] = qMin(1.0, m_keyHeat.value(keyId, 0.0) + 0.38);
    m_ripples.append({position, 0.0});
    emit frameChanged();
}

void AnimationEngine::clearReactiveState()
{
    m_keyHeat.clear();
    m_ripples.clear();
    emit frameChanged();
}

void AnimationEngine::advance(qreal seconds)
{
    const qreal delta = qBound(0.0, seconds, 0.25);
    if (m_paused || delta == 0.0) return;
    const qreal speed = 0.25 + m_profile.speed / 35.0;
    m_phase = unitPhase(m_phase + delta * speed * (m_reverse ? -1.0 : 1.0));
    if (m_duration > 0.0) {
        m_elapsed += delta;
        if (m_elapsed >= m_duration) {
            if (m_looping) m_elapsed = std::fmod(m_elapsed, m_duration);
            else {
                m_elapsed = m_duration;
                m_paused = true;
            }
        }
    }

    const qreal cooling = qExp(-delta / qMax(0.1, m_decaySeconds));
    for (auto it = m_keyHeat.begin(); it != m_keyHeat.end();) {
        it.value() *= cooling;
        if (it.value() < 0.01) it = m_keyHeat.erase(it);
        else ++it;
    }
    for (Ripple &ripple : m_ripples) ripple.age += delta;
    for (qsizetype i = m_ripples.size() - 1; i >= 0; --i) {
        if (m_ripples[i].age > m_decaySeconds * 2.0) m_ripples.removeAt(i);
    }
    emit frameChanged();
}

std::array<QColor, 4> AnimationEngine::frame() const
{
    auto out = m_profile.zones;
    qreal brightness = m_profile.brightness / 100.0;
    if (m_duration > 0.0) {
        if (m_fadeInSeconds > 0.0 && m_elapsed < m_fadeInSeconds) {
            brightness *= qBound(0.0, m_elapsed / m_fadeInSeconds, 1.0);
        }
        if (!m_looping && m_fadeOutSeconds > 0.0 &&
            m_elapsed > m_duration - m_fadeOutSeconds) {
            brightness *= qBound(0.0, (m_duration - m_elapsed) / m_fadeOutSeconds, 1.0);
        }
    }

    if (m_previewEffect == PreviewEffect::AudioVisualizer && !m_audioLevel) return out;
    if (m_previewEffect == PreviewEffect::SystemMonitor && !m_systemLoad) return out;

    const bool effectIsProfile = m_previewEffect == PreviewEffect::Profile;
    const PreviewEffect effect = effectIsProfile
                                     ? (m_profile.effect == Effect::Static ? PreviewEffect::Profile
                                        : m_profile.effect == Effect::Breathing ? PreviewEffect::Breathing
                                        : m_profile.effect == Effect::Wave ? PreviewEffect::Wave
                                        : m_profile.effect == Effect::Hue ? PreviewEffect::Rainbow
                                                                          : PreviewEffect::Profile)
                                     : m_previewEffect;

    switch (effect) {
    case PreviewEffect::Profile:
        for (QColor &color : out) color = scaleColor(color, brightness);
        return out;
    case PreviewEffect::Breathing: {
        const qreal pulse = 1.0 - m_breathingDepth *
                                  (0.5 + 0.5 * qCos(m_phase * 2.0 * M_PI));
        for (int i = 0; i < 4; ++i) {
            const QColor base = m_presetColorMode == PresetColorMode::Multi
                ? hueColor(m_phase + i / 4.0, 0.94, 1.0)
                : out[static_cast<size_t>(i)];
            out[static_cast<size_t>(i)] = scaleColor(base, pulse * brightness);
        }
        return out;
    }
    case PreviewEffect::Wave:
    case PreviewEffect::PingPong:
    case PreviewEffect::CenterOut:
    case PreviewEffect::OutsideIn:
    case PreviewEffect::VerticalWave:
    case PreviewEffect::BidirectionalWave:
    case PreviewEffect::AlternatingDirection:
        for (int i = 0; i < 4; ++i) {
            qreal coordinate = static_cast<qreal>(i) / 3.0;
            if (m_profile.direction == Direction::RightToLeft) coordinate = 1.0 - coordinate;
            if (effect == PreviewEffect::CenterOut || effect == PreviewEffect::OutsideIn)
                coordinate = qAbs(0.5 - coordinate) * 2.0;
            if (effect == PreviewEffect::OutsideIn) coordinate = 1.0 - coordinate;
            qreal phase = m_phase;
            if (effect == PreviewEffect::PingPong || effect == PreviewEffect::AlternatingDirection) {
                const qreal triangle = 1.0 - qAbs(2.0 * unitPhase(m_phase) - 1.0);
                phase = triangle;
            }
            const qreal wave = 0.18 + 0.82 * (0.5 + 0.5 * qCos((coordinate - phase) * 2.0 * M_PI));
            if (effect == PreviewEffect::Wave && m_presetColorMode == PresetColorMode::Multi) {
                out[static_cast<size_t>(i)] =
                    hueColor(m_phase + coordinate * 0.25, 0.94, wave * brightness);
            } else {
                out[static_cast<size_t>(i)] =
                    scaleColor(out[static_cast<size_t>(i)], wave * brightness);
            }
        }
        return out;
    case PreviewEffect::Rainbow:
        for (int i = 0; i < 4; ++i) {
            out[static_cast<size_t>(i)] = m_customRainbowPalette
                ? scaleColor(m_palette.sample(unitPhase(m_phase + i / 4.0)), brightness)
                : hueColor(m_phase + i / 4.0, m_rainbowSaturation, brightness);
        }
        return out;
    case PreviewEffect::Fire:
        for (int i = 0; i < 4; ++i) {
            const qreal noise = 0.5 + 0.5 * qSin((m_phase * 2.0 + i * 0.61) * 2.0 * M_PI);
            const qreal flicker = 0.25 + 0.75 * noise;
            out[static_cast<size_t>(i)] = hueColor(0.015 + 0.09 * flicker, 0.96, flicker * brightness);
        }
        return out;
    case PreviewEffect::Aurora:
        for (int i = 0; i < 4; ++i) {
            const qreal position = unitPhase(m_phase * 0.35 + i / 3.0);
            const QColor c = m_palette.sample(position);
            out[static_cast<size_t>(i)] = scaleColor(c, brightness);
        }
        return out;
    case PreviewEffect::ColorPulse: {
        const qreal pulse = 0.12 + 0.88 * qPow(0.5 + 0.5 * qSin(m_phase * 2.0 * M_PI), 2.0);
        for (int i = 0; i < 4; ++i) {
            out[static_cast<size_t>(i)] = scaleColor(out[static_cast<size_t>(i)], pulse * brightness);
        }
        return out;
    }
    case PreviewEffect::AlternatingZones: {
        const qreal pulse = 0.15 + 0.85 * (0.5 + 0.5 * qSin(m_phase * 2.0 * M_PI));
        for (int i = 0; i < 4; ++i) {
            const qreal level = ((i + static_cast<int>(m_phase * 4.0)) % 2 == 0) ? pulse : 0.12;
            out[static_cast<size_t>(i)] = scaleColor(out[static_cast<size_t>(i)], level * brightness);
        }
        return out;
    }
    case PreviewEffect::ColorTransition:
        for (int i = 0; i < 4; ++i) {
            const qreal amount = 0.5 - 0.5 * qCos(m_phase * 2.0 * M_PI);
            const QColor a = m_profile.zones[static_cast<size_t>(i)];
            const QColor b = m_profile.zones[static_cast<size_t>((i + 1) % 4)];
            out[static_cast<size_t>(i)] = scaleColor(ColorGradient::interpolate(a, b, amount), brightness);
        }
        return out;
    case PreviewEffect::AudioVisualizer: {
        const qreal level = qBound(0.0, *m_audioLevel, 1.0);
        for (int i = 0; i < 4; ++i) {
            out[static_cast<size_t>(i)] = scaleColor(out[static_cast<size_t>(i)],
                                                      (0.12 + 0.88 * level * (i + 1) / 4.0) * brightness);
        }
        return out;
    }
    case PreviewEffect::SystemMonitor: {
        const qreal level = qBound(0.0, *m_systemLoad, 1.0);
        for (int i = 0; i < 4; ++i) {
            out[static_cast<size_t>(i)] = hueColor(0.33 * (1.0 - level), 0.9,
                                                    ((i + 1) / 4.0 <= level) ? brightness : 0.08);
        }
        return out;
    }
    case PreviewEffect::Ripple:
    case PreviewEffect::WaterRipple:
    case PreviewEffect::KeyGlow:
    case PreviewEffect::HeatMap:
        for (QColor &color : out) color = scaleColor(color, brightness);
        return out;
    }
    return out;
}

QHash<int, QColor> AnimationEngine::keyFrame(const KeyboardLayout &layout,
                                             const KeyboardZoneMap &zoneMap) const
{
    QHash<int, QColor> result;
    const PreviewEffect effect = m_previewEffect;
    const bool reactive = effect == PreviewEffect::Ripple || effect == PreviewEffect::WaterRipple ||
                          effect == PreviewEffect::KeyGlow || effect == PreviewEffect::HeatMap;
    const bool animated = effect == PreviewEffect::Rainbow || effect == PreviewEffect::Wave ||
                          effect == PreviewEffect::Aurora || effect == PreviewEffect::Fire ||
                          effect == PreviewEffect::CenterOut || effect == PreviewEffect::PingPong ||
                          effect == PreviewEffect::OutsideIn || effect == PreviewEffect::VerticalWave ||
                          effect == PreviewEffect::BidirectionalWave ||
                          effect == PreviewEffect::AlternatingDirection ||
                          effect == PreviewEffect::ColorPulse ||
                          effect == PreviewEffect::AlternatingZones ||
                          effect == PreviewEffect::ColorTransition;
    if (!reactive && !animated) {
        return result;
    }
    const auto zoneColors = frame();
    for (const KeyGeometry &key : layout.keys()) {
        const int zone = qBound(0, zoneMap.zoneForKey(key.id), 3);
        QColor color = zoneColors[static_cast<size_t>(zone)];
        const qreal x = (key.x + key.width / 2.0) / qMax(1.0, layout.totalWidth());
        const qreal y = (key.y + key.height / 2.0) / qMax(1.0, layout.totalHeight());

        if (animated) {
            switch (effect) {
            case PreviewEffect::Rainbow:
                color = m_customRainbowPalette
                            ? m_palette.sample(unitPhase(m_phase + x))
                            : hueColor(m_phase + x, m_rainbowSaturation, m_profile.brightness / 100.0);
                break;
            case PreviewEffect::Wave:
            case PreviewEffect::PingPong:
            case PreviewEffect::AlternatingDirection:
            case PreviewEffect::VerticalWave:
            case PreviewEffect::BidirectionalWave: {
                const qreal position = m_profile.direction == Direction::RightToLeft ? 1.0 - x : x;
                const qreal axis = effect == PreviewEffect::VerticalWave ? y : position;
                const qreal phase = (effect == PreviewEffect::PingPong ||
                                     effect == PreviewEffect::AlternatingDirection)
                    ? 1.0 - qAbs(2.0 * unitPhase(m_phase) - 1.0) : m_phase;
                const qreal wave = effect == PreviewEffect::BidirectionalWave
                    ? 0.18 + 0.82 * (0.5 + 0.5 * qCos((axis - phase) * 4.0 * M_PI))
                    : 0.18 + 0.82 * (0.5 + 0.5 * qCos((axis - phase) * 2.0 * M_PI));
                color = scaleColor(m_profile.zones[static_cast<size_t>(zone)],
                                   wave * m_profile.brightness / 100.0);
                break;
            }
            case PreviewEffect::CenterOut: {
                const qreal distance = qAbs(x - 0.5) * 2.0;
                const qreal wave = 0.18 + 0.82 * (0.5 + 0.5 *
                    qCos((distance - m_phase) * 2.0 * M_PI));
                color = scaleColor(m_profile.zones[static_cast<size_t>(zone)],
                                   wave * m_profile.brightness / 100.0);
                break;
            }
            case PreviewEffect::OutsideIn: {
                const qreal distance=1.0-qAbs(x-0.5)*2.0;
                const qreal wave=0.18+0.82*(0.5+0.5*qCos((distance-m_phase)*2.0*M_PI));
                color=scaleColor(m_profile.zones[static_cast<size_t>(zone)],
                                 wave*m_profile.brightness/100.0);
                break;
            }
            case PreviewEffect::Aurora:
                color = scaleColor(m_palette.sample(unitPhase(m_phase * 0.2 + x +
                    0.12 * qSin(y * 4.0 + m_phase * 2.0 * M_PI))),
                    m_profile.brightness / 100.0);
                break;
            case PreviewEffect::Fire: {
                const qreal flicker = 0.25 + 0.75 * (0.5 + 0.5 *
                    qSin(m_phase * 2.0 * M_PI + key.id * 1.31 + x * 9.0));
                color = hueColor(0.015 + 0.09 * flicker, 0.96,
                                 flicker * m_profile.brightness / 100.0);
                break;
            }
            case PreviewEffect::ColorPulse: {
                const qreal pulse = 0.12 + 0.88 * qPow(0.5 + 0.5 *
                    qSin(m_phase * 2.0 * M_PI), 2.0);
                color = scaleColor(m_profile.zones[static_cast<size_t>(zone)],
                                   pulse * m_profile.brightness / 100.0);
                break;
            }
            case PreviewEffect::AlternatingZones: {
                const int band = static_cast<int>(x * 8.0);
                const qreal level = (band + static_cast<int>(m_phase * 4.0)) % 2 == 0 ? 1.0 : 0.2;
                color = scaleColor(m_profile.zones[static_cast<size_t>(zone)],
                                   level * m_profile.brightness / 100.0);
                break;
            }
            case PreviewEffect::ColorTransition: {
                const qreal amount = 0.5 - 0.5 * qCos(m_phase * 2.0 * M_PI);
                const QColor left = m_palette.sample(x);
                const QColor right = m_palette.sample(unitPhase(x + 0.25));
                color = scaleColor(ColorGradient::interpolate(left, right, amount),
                                   m_profile.brightness / 100.0);
                break;
            }
            default:
                break;
            }
        }

        qreal amount = reactive ? m_keyHeat.value(key.id, 0.0) : 0.0;
        if (reactive && effect == PreviewEffect::HeatMap) {
            amount = m_keyHeat.value(key.id, 0.0);
            const QColor hot = m_palette.sample(amount);
            color = ColorGradient::interpolate(color, hot, amount);
        }
        if (reactive && (effect == PreviewEffect::Ripple || effect == PreviewEffect::WaterRipple)) {
            const QPointF center(key.x + key.width / 2.0, key.y + key.height / 2.0);
            for (const Ripple &ripple : m_ripples) {
                const qreal distance = QLineF(center, ripple.position).length();
                const qreal radius = qMin(m_rippleMaxRadius, ripple.age * m_rippleSpeed);
                qreal ring = 0.0;
                for (int wave = 0; wave < m_waveCount; ++wave) {
                    const qreal waveRadius = radius - wave * 1.2;
                    ring = qMax(ring, qExp(-qAbs(distance - waveRadius) * 0.55) *
                                         qExp(-ripple.age / qMax(0.1, m_decaySeconds)));
                }
                amount = qMax(amount, ring);
            }
        }
        if (amount > 0.01) {
            const QColor overlay = effect == PreviewEffect::HeatMap ? color : m_reactiveColor;
            color = ColorGradient::interpolate(color, overlay,
                                               qBound(0.0, amount * m_reactiveIntensity, 1.0));
        }
        result.insert(key.id, color);
    }
    return result;
}
