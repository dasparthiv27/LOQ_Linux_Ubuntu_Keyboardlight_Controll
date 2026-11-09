#pragma once
#include "profile.h"
#include <QByteArray>
#include <QObject>
#include <QString>

struct HardwareInfo { QString name="Lenovo LOQ 15AHP9"; QString machineType="83DX"; quint16 vendorId=0x048D; quint16 productId=0xC993; int rgbInterface=0; int genericInterface=1; QString controller="ITE8295"; };
class KeyboardBackend {
public:
    virtual ~KeyboardBackend()=default;
    virtual bool isSimulation() const=0;
    virtual bool apply(const Profile &profile,QString *error=nullptr)=0;
    virtual HardwareInfo info() const=0;
};
class SimulationBackend final: public KeyboardBackend {
public:
    bool isSimulation() const override{return true;}
    bool apply(const Profile &profile,QString *error=nullptr) override;
    HardwareInfo info() const override{return {};}
    Profile lastApplied() const{return m_last;}
private: Profile m_last;
};

class HidrawTransport {
public:
    virtual ~HidrawTransport() = default;
    virtual QString discoverInterface0(QString *error = nullptr) = 0;
    virtual bool sendFeatureReport(const QByteArray &report,
                                   QString *error = nullptr) = 0;
};

class LinuxHidrawTransport final : public HidrawTransport {
public:
    explicit LinuxHidrawTransport(QString sysClassPath = QStringLiteral("/sys/class/hidraw"),
                                 QString deviceRoot = QStringLiteral("/dev"));
    QString discoverInterface0(QString *error = nullptr) override;
    bool sendFeatureReport(const QByteArray &report,
                           QString *error = nullptr) override;
    static bool matchesRawIdentity(quint32 busType, qint16 vendor, qint16 product);

private:
    QString m_sysClassPath;
    QString m_deviceRoot;
};

class Ite8295Backend final : public KeyboardBackend {
public:
    explicit Ite8295Backend(HidrawTransport *transport = nullptr);
    ~Ite8295Backend() override;

    bool isSimulation() const override { return false; }
    bool apply(const Profile &profile, QString *error = nullptr) override;
    HardwareInfo info() const override { return {}; }

    static QByteArray buildFeatureReport(const Profile &profile,
                                         QString *error = nullptr);
    static int featureDataLength(const QByteArray &reportDescriptor,
                                 unsigned char reportId);

private:
    HidrawTransport *m_transport = nullptr;
    bool m_ownsTransport = false;
};

class HardwareApplyWorker final : public QObject {
    Q_OBJECT
public:
    explicit HardwareApplyWorker(QObject *parent = nullptr);

public slots:
    void applyProfile(const Profile &profile);

signals:
    void applyFinished(const Profile &profile, bool success, const QString &message);

private:
    Ite8295Backend m_backend;
};
