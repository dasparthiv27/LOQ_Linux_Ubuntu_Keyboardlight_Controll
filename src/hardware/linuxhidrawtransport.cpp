#include "keyboardbackend.h"
#include "ite8295protocol.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

Q_LOGGING_CATEGORY(hidrawLog, "loq.hidraw")

constexpr quint16 VendorId = 0x048D;
constexpr quint16 ProductId = 0xC993;

bool readTextFile(const QString &path, QString *value)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    *value = QString::fromLatin1(file.readAll()).trimmed();
    return true;
}

QString interfaceDirectory(const QString &hidDevicePath)
{
    QDir current(hidDevicePath);
    const QRegularExpression interfaceName(QStringLiteral("^.+:[0-9]+\\.0$"));
    while (current.exists()) {
        if (interfaceName.match(current.dirName()).hasMatch()) {
            return current.absolutePath();
        }
        if (!current.cdUp()) {
            break;
        }
    }
    return {};
}

QString candidateError(const QString &path, const QString &reason)
{
    return QString("%1: %2").arg(path, reason);
}

QString sysRootForClassPath(const QString &sysClassPath)
{
    QDir sysRoot(sysClassPath);
    if (!sysRoot.cdUp() || !sysRoot.cdUp()) {
        return {};
    }
    return sysRoot.absolutePath();
}

bool isExpectedInterface0(const QString &hidDevicePath, QString *error)
{
    const QString interfacePath = interfaceDirectory(hidDevicePath);
    if (interfacePath.isEmpty()) {
        if (error) {
            *error = QStringLiteral("HID sysfs path has no USB interface ancestor.");
        }
        return false;
    }

    QString interfaceNumber;
    if (!readTextFile(interfacePath + QStringLiteral("/bInterfaceNumber"), &interfaceNumber) ||
        interfaceNumber != QStringLiteral("00")) {
        if (error) {
            *error = QStringLiteral("HID node is not USB interface 0.");
        }
        return false;
    }

    const QString usbDevicePath = QFileInfo(interfacePath).dir().absolutePath();
    QString vendor;
    QString product;
    if (!readTextFile(usbDevicePath + QStringLiteral("/idVendor"), &vendor) ||
        !readTextFile(usbDevicePath + QStringLiteral("/idProduct"), &product) ||
        vendor.compare(QStringLiteral("048d"), Qt::CaseInsensitive) != 0 ||
        product.compare(QStringLiteral("c993"), Qt::CaseInsensitive) != 0) {
        if (error) {
            *error = QStringLiteral("USB sysfs identity is not 048D:C993.");
        }
        return false;
    }

    QByteArray descriptor;
    QFile descriptorFile(hidDevicePath + QStringLiteral("/report_descriptor"));
    if (!descriptorFile.open(QIODevice::ReadOnly) ||
        !(descriptor = descriptorFile.readAll()).size()) {
        if (error) {
            *error = QStringLiteral("Cannot read interface-0 HID report descriptor.");
        }
        return false;
    }
    if (Ite8295Protocol::featureDataLength(descriptor, Ite8295Protocol::ReportId) !=
        Ite8295Protocol::FeatureDataLength) {
        if (error) {
            *error = QStringLiteral("Interface-0 Feature report 0xCC is not 32 data bytes.");
        }
        return false;
    }
    return true;
}

} // namespace

LinuxHidrawTransport::LinuxHidrawTransport(QString sysClassPath, QString deviceRoot)
    : m_sysClassPath(std::move(sysClassPath)),
      m_deviceRoot(std::move(deviceRoot))
{
}

QString LinuxHidrawTransport::discoverInterface0(QString *error)
{
    QDir sysClass(m_sysClassPath);
    const auto entries = sysClass.entryInfoList(
        QStringList{QStringLiteral("hidraw*")}, QDir::Dirs | QDir::System | QDir::NoDotAndDotDot);
    QStringList rejected;

    for (const QFileInfo &entry : entries) {
        const QString hidDevicePath =
            QFileInfo(entry.absoluteFilePath() + QStringLiteral("/device")).canonicalFilePath();
        if (hidDevicePath.isEmpty()) {
            rejected.append(candidateError(entry.fileName(), "cannot resolve HID sysfs device"));
            continue;
        }

        QString identityError;
        if (!isExpectedInterface0(hidDevicePath, &identityError)) {
            if (identityError != QStringLiteral("HID node is not USB interface 0.")) {
                rejected.append(candidateError(entry.fileName(), identityError));
            }
            continue;
        }

        const QString devicePath = QDir(m_deviceRoot).filePath(entry.fileName());
        if (!QFileInfo::exists(devicePath)) {
            rejected.append(candidateError(entry.fileName(), "device node is missing"));
            continue;
        }
        if (error) {
            error->clear();
        }
        return devicePath;
    }

    if (error) {
        *error = rejected.isEmpty()
                     ? QStringLiteral("ITE8295 interface 0 (048D:C993) was not found.")
                     : QStringLiteral("No valid ITE8295 interface-0 node found: %1")
                           .arg(rejected.join(QStringLiteral("; ")));
    }
    return {};
}

bool LinuxHidrawTransport::matchesRawIdentity(quint32 busType, qint16 vendor, qint16 product)
{
    return busType == BUS_USB &&
           static_cast<quint16>(vendor) == VendorId &&
           static_cast<quint16>(product) == ProductId;
}

bool LinuxHidrawTransport::sendFeatureReport(const QByteArray &report,
                                              QString *error)
{
    if (report.size() != Ite8295Protocol::HidrawBufferLength ||
        static_cast<unsigned char>(report.at(0)) != Ite8295Protocol::ReportId) {
        if (error) {
            *error = QStringLiteral("Refusing malformed ITE8295 Feature report.");
        }
        return false;
    }

    QString discoveryError;
    const QString devicePath = discoverInterface0(&discoveryError);
    if (devicePath.isEmpty()) {
        if (error) {
            *error = discoveryError;
        }
        return false;
    }

    const QByteArray nativePath = QFile::encodeName(devicePath);
    const int fd = open(nativePath.constData(), O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        if (error) {
            *error = QStringLiteral("Cannot open %1 read/write: %2")
                         .arg(devicePath, QString::fromLocal8Bit(std::strerror(errno)));
        }
        return false;
    }

    struct stat openedNode {};
    const int statResult = fstat(fd, &openedNode);
    if (statResult < 0 || !S_ISCHR(openedNode.st_mode)) {
        const int savedError = errno;
        close(fd);
        if (error) {
            *error = statResult < 0
                         ? QStringLiteral("Cannot verify character device %1: %2")
                               .arg(devicePath, QString::fromLocal8Bit(std::strerror(savedError)))
                         : QStringLiteral("%1 is not a character device.").arg(devicePath);
        }
        return false;
    }

    hidraw_devinfo rawInfo{};
    if (ioctl(fd, HIDIOCGRAWINFO, &rawInfo) < 0) {
        const int savedError = errno;
        close(fd);
        if (error) {
            *error = QStringLiteral("Cannot verify hidraw identity for %1: %2")
                         .arg(devicePath, QString::fromLocal8Bit(std::strerror(savedError)));
        }
        return false;
    }
    if (!matchesRawIdentity(rawInfo.bustype, rawInfo.vendor, rawInfo.product)) {
        close(fd);
        if (error) {
            *error = QStringLiteral("Opened hidraw node is not USB 048D:C993; refusing Feature SET.");
        }
        return false;
    }

    const QString sysRoot = sysRootForClassPath(m_sysClassPath);
    const QString charDeviceLink =
        QStringLiteral("%1/dev/char/%2:%3")
            .arg(sysRoot)
            .arg(major(openedNode.st_rdev))
            .arg(minor(openedNode.st_rdev));
    const QString openedHidrawPath = QFileInfo(charDeviceLink).canonicalFilePath();
    const QString openedHidDevicePath =
        QFileInfo(openedHidrawPath + QStringLiteral("/device")).canonicalFilePath();
    QString openedInterfaceError;
    if (sysRoot.isEmpty() || openedHidrawPath.isEmpty() ||
        QFileInfo(openedHidrawPath).fileName() != QFileInfo(devicePath).fileName() ||
        !isExpectedInterface0(openedHidDevicePath, &openedInterfaceError)) {
        close(fd);
        if (error) {
            *error = QStringLiteral("Opened hidraw node no longer maps to verified USB 048D:C993 interface 0 (%1); refusing Feature SET.")
                         .arg(openedInterfaceError.isEmpty()
                                  ? QStringLiteral("sysfs node identity changed")
                                  : openedInterfaceError);
        }
        return false;
    }

    const int result = ioctl(fd, HIDIOCSFEATURE(Ite8295Protocol::HidrawBufferLength),
                             const_cast<char *>(report.constData()));
    const int savedError = errno;
    const int closeResult = close(fd);
    if (result < 0) {
        if (error) {
            *error = QStringLiteral("HIDIOCSFEATURE(%1) failed for %2: %3")
                         .arg(Ite8295Protocol::HidrawBufferLength)
                         .arg(devicePath, QString::fromLocal8Bit(std::strerror(savedError)));
        }
        return false;
    }
    if (result != Ite8295Protocol::HidrawBufferLength) {
        if (error) {
            *error = QStringLiteral("HIDIOCSFEATURE returned %1 bytes; expected %2.")
                         .arg(result)
                         .arg(Ite8295Protocol::HidrawBufferLength);
        }
        return false;
    }
    if (closeResult < 0) {
        qCWarning(hidrawLog) << "Feature SET completed, but closing" << devicePath
                             << "failed:" << QString::fromLocal8Bit(std::strerror(errno));
    }
    return true;
}
