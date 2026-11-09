#pragma once

#include "profile.h"
#include <QByteArray>
#include <QString>

class Ite8295Protocol {
public:
    static constexpr unsigned char ReportId = 0xCC;
    static constexpr int FeatureDataLength = 32;
    static constexpr int HidrawBufferLength = FeatureDataLength + 1;

    static int featureDataLength(const QByteArray &reportDescriptor,
                                 unsigned char reportId);
    static QByteArray buildFeatureReport(const Profile &profile,
                                         QString *error = nullptr);
};
