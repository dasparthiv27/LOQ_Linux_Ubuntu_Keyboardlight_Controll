#include "keyboardbackend.h"
#include "ite8295protocol.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(backendLog,"loq.backend")

bool SimulationBackend::apply(const Profile &profile, QString *error)
{
    if (!profile.isValid(error)) {
        qCWarning(backendLog) << "Rejected invalid simulation profile:"
                              << (error ? *error : QString());
        return false;
    }
    m_last = profile;
    qCInfo(backendLog) << "Applied profile in simulation:" << profile.name
                       << '(' << effectName(profile.effect) << ')';
    return true;
}

Ite8295Backend::Ite8295Backend(HidrawTransport *transport)
    : m_transport(transport ? transport : new LinuxHidrawTransport),
      m_ownsTransport(!transport)
{
}

Ite8295Backend::~Ite8295Backend()
{
    if (m_ownsTransport) {
        delete m_transport;
    }
}

QByteArray Ite8295Backend::buildFeatureReport(const Profile &profile, QString *error)
{
    return Ite8295Protocol::buildFeatureReport(profile, error);
}

int Ite8295Backend::featureDataLength(const QByteArray &descriptor, unsigned char reportId)
{
    return Ite8295Protocol::featureDataLength(descriptor, reportId);
}

bool Ite8295Backend::apply(const Profile &profile, QString *error)
{
    const QByteArray report = Ite8295Protocol::buildFeatureReport(profile, error);
    if (report.isEmpty()) {
        return false;
    }

    if (!m_transport->sendFeatureReport(report, error)) {
        qCWarning(backendLog) << "ITE8295 apply failed:" << (error ? *error : QString());
        return false;
    }

    qCInfo(backendLog) << "ITE8295 Feature SET transfer completed for verified interface 0";
    return true;
}

HardwareApplyWorker::HardwareApplyWorker(QObject *parent)
    : QObject(parent)
{
}

void HardwareApplyWorker::applyProfile(const Profile &profile)
{
    QString error;
    const bool success = m_backend.apply(profile, &error);
    emit applyFinished(profile, success,
                       success ? QStringLiteral("Feature SET transfer completed.")
                               : error);
}
