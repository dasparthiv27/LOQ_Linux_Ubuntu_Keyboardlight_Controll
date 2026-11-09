#include "ite8295protocol.h"

#include <QHash>
#include <QVector>
#include <limits>

namespace {

bool fail(QString *error, const QString &message)
{
    if (error) {
        *error = message;
    }
    return false;
}

int speedLevel(int speed)
{
    return 1 + ((speed - 1) * 3 + 49) / 99;
}

int brightnessLevel(int brightness)
{
    return brightness < 50 ? 1 : 2;
}

unsigned char scaledChannel(int channel, int brightness)
{
    return static_cast<unsigned char>(brightness == 1 ? (channel * 3) / 5 : channel);
}

} // namespace

int Ite8295Protocol::featureDataLength(const QByteArray &descriptor,
                                      unsigned char targetReportId)
{
    struct GlobalState {
        unsigned int reportSize = 0;
        unsigned int reportCount = 0;
        unsigned char reportId = 0;
    };

    GlobalState state;
    QVector<GlobalState> globalStack;
    QHash<unsigned int, quint64> featureBits;

    for (qsizetype offset = 0; offset < descriptor.size();) {
        const auto prefix = static_cast<unsigned char>(descriptor.at(offset++));
        if (prefix == 0xFE) {
            if (offset + 2 > descriptor.size()) {
                return -1;
            }
            const auto payloadLength = static_cast<unsigned char>(descriptor.at(offset));
            offset += 2;
            if (offset + payloadLength > descriptor.size()) {
                return -1;
            }
            offset += payloadLength;
            continue;
        }

        const unsigned int sizeCode = prefix & 0x03;
        const unsigned int dataLength = sizeCode == 3 ? 4 : sizeCode;
        if (offset + dataLength > descriptor.size()) {
            return -1;
        }

        quint32 value = 0;
        for (unsigned int i = 0; i < dataLength; ++i) {
            value |= static_cast<quint32>(
                         static_cast<unsigned char>(descriptor.at(offset + i)))
                     << (8 * i);
        }
        offset += dataLength;

        const unsigned int itemType = (prefix >> 2) & 0x03;
        const unsigned int itemTag = (prefix >> 4) & 0x0F;
        if (itemType == 1) {
            switch (itemTag) {
            case 7:
                state.reportSize = value;
                break;
            case 8:
                if (value == 0 || value > 0xFF) {
                    return -1;
                }
                state.reportId = static_cast<unsigned char>(value);
                break;
            case 9:
                state.reportCount = value;
                break;
            case 10:
                globalStack.append(state);
                break;
            case 11:
                if (globalStack.isEmpty()) {
                    return -1;
                }
                state = globalStack.takeLast();
                break;
            default:
                break;
            }
        } else if (itemType == 0 && itemTag == 11) {
            featureBits[state.reportId] +=
                static_cast<quint64>(state.reportSize) * state.reportCount;
        }
    }

    if (!featureBits.contains(targetReportId) || featureBits[targetReportId] == 0) {
        return -1;
    }
    const quint64 bits = featureBits.value(targetReportId);
    const quint64 bytes = (bits + 7) / 8;
    return bytes > static_cast<quint64>(std::numeric_limits<int>::max())
               ? -1
               : static_cast<int>(bytes);
}

QByteArray Ite8295Protocol::buildFeatureReport(const Profile &profile, QString *error)
{
    QString validationError;
    if (!profile.isValid(&validationError)) {
        fail(error, validationError);
        return {};
    }
    if (profile.brightness == 0) {
        fail(error, "The reference protocol exposes only two nonzero brightness levels.");
        return {};
    }

    unsigned char effect = 0;
    switch (profile.effect) {
    case Effect::Static:
        effect = 0x01;
        break;
    case Effect::Breathing:
        effect = 0x03;
        break;
    case Effect::Wave:
        effect = 0x04;
        break;
    case Effect::Hue:
        effect = 0x06;
        break;
    default:
        fail(error, "This effect is not represented by the ITE8295 reference protocol.");
        return {};
    }

    const int brightness = brightnessLevel(profile.brightness);
    QByteArray report(HidrawBufferLength, '\0');
    report[0] = static_cast<char>(ReportId);
    report[1] = static_cast<char>(0x16);
    report[2] = static_cast<char>(effect);
    report[3] = static_cast<char>(speedLevel(profile.speed));
    report[4] = static_cast<char>(brightness);

    if (profile.effect == Effect::Static || profile.effect == Effect::Breathing) {
        for (int zone = 0; zone < 4; ++zone) {
            const QColor color = profile.zones[static_cast<size_t>(zone)];
            const int offset = 5 + zone * 3;
            report[offset] = static_cast<char>(scaledChannel(color.red(), brightness));
            report[offset + 1] = static_cast<char>(scaledChannel(color.green(), brightness));
            report[offset + 2] = static_cast<char>(scaledChannel(color.blue(), brightness));
        }
    }

    if (profile.effect == Effect::Wave) {
        if (profile.direction == Direction::RightToLeft) {
            report[18] = 1;
        } else {
            report[19] = 1;
        }
    }

    return report;
}
