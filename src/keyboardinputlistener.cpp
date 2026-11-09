#include "keyboardinputlistener.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace {

bool belongsToIte8295(const QString &path)
{
    QDir current(path);
    while(current.exists()){
        QFile vendorFile(current.filePath(QStringLiteral("idVendor")));
        QFile productFile(current.filePath(QStringLiteral("idProduct")));
        if(vendorFile.open(QIODevice::ReadOnly|QIODevice::Text)&&
           productFile.open(QIODevice::ReadOnly|QIODevice::Text)){
            const QString vendor=QString::fromLatin1(vendorFile.readAll()).trimmed();
            const QString product=QString::fromLatin1(productFile.readAll()).trimmed();
            if(vendor.compare(QStringLiteral("048d"),Qt::CaseInsensitive)==0&&
               product.compare(QStringLiteral("c993"),Qt::CaseInsensitive)==0)
                return true;
        }
        if(!current.cdUp())break;
    }
    return false;
}

} // namespace

KeyboardInputListener::KeyboardInputListener(QObject *parent) : QThread(parent) {}

KeyboardInputListener::~KeyboardInputListener()
{
    stop();
    wait();
}

QList<KeyboardInputDevice> KeyboardInputListener::availableKeyboards()
{
    QList<KeyboardInputDevice> result;
    const QDir inputClass(QStringLiteral("/sys/class/input"));
    const QStringList entries = inputClass.entryList({QStringLiteral("event*")},
                                                       QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        const QString sysPath = QFileInfo(inputClass.filePath(entry + QStringLiteral("/device")))
                                    .canonicalFilePath();
        if(sysPath.isEmpty()||belongsToIte8295(sysPath))continue;
        QFile nameFile(sysPath + QStringLiteral("/name"));
        if (!nameFile.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        const QString name = QString::fromLocal8Bit(nameFile.readAll()).trimmed();
        if (!name.contains(QStringLiteral("keyboard"), Qt::CaseInsensitive)) continue;
        result.append({QStringLiteral("/dev/input/") + entry, name});
    }
    return result;
}

QString KeyboardInputListener::keyLabelForCode(unsigned int code)
{
    if (code >= KEY_1 && code <= KEY_9) {
        static const char *labels[] = {"1","2","3","4","5","6","7","8","9"};
        return QString::fromLatin1(labels[code - KEY_1]);
    }
    switch (code) {
    case KEY_A: return QStringLiteral("A");
    case KEY_B: return QStringLiteral("B");
    case KEY_C: return QStringLiteral("C");
    case KEY_D: return QStringLiteral("D");
    case KEY_E: return QStringLiteral("E");
    case KEY_F: return QStringLiteral("F");
    case KEY_G: return QStringLiteral("G");
    case KEY_H: return QStringLiteral("H");
    case KEY_I: return QStringLiteral("I");
    case KEY_J: return QStringLiteral("J");
    case KEY_K: return QStringLiteral("K");
    case KEY_L: return QStringLiteral("L");
    case KEY_M: return QStringLiteral("M");
    case KEY_N: return QStringLiteral("N");
    case KEY_O: return QStringLiteral("O");
    case KEY_P: return QStringLiteral("P");
    case KEY_Q: return QStringLiteral("Q");
    case KEY_R: return QStringLiteral("R");
    case KEY_S: return QStringLiteral("S");
    case KEY_T: return QStringLiteral("T");
    case KEY_U: return QStringLiteral("U");
    case KEY_V: return QStringLiteral("V");
    case KEY_W: return QStringLiteral("W");
    case KEY_X: return QStringLiteral("X");
    case KEY_Y: return QStringLiteral("Y");
    case KEY_Z: return QStringLiteral("Z");
    case KEY_0: return QStringLiteral("0");
    case KEY_ESC: return QStringLiteral("Esc");
    case KEY_TAB: return QStringLiteral("Tab");
    case KEY_CAPSLOCK: return QStringLiteral("Caps");
    case KEY_ENTER: return QStringLiteral("Enter");
    case KEY_BACKSPACE: return QStringLiteral("Backspace");
    case KEY_LEFTSHIFT: return QStringLiteral("LShift");
    case KEY_RIGHTSHIFT: return QStringLiteral("RShift");
    case KEY_SPACE: return QStringLiteral("Space");
    case KEY_LEFTCTRL: return QStringLiteral("Ctrl");
    case KEY_LEFTALT: return QStringLiteral("Alt");
    case KEY_RIGHTALT: return QStringLiteral("Alt");
    case KEY_LEFTMETA: return QStringLiteral("Win");
    case KEY_UP: return QStringLiteral("▲");
    case KEY_DOWN: return QStringLiteral("▼");
    case KEY_LEFT: return QStringLiteral("◄");
    case KEY_RIGHT: return QStringLiteral("►");
    case KEY_F1: return QStringLiteral("F1");
    case KEY_F2: return QStringLiteral("F2");
    case KEY_F3: return QStringLiteral("F3");
    case KEY_F4: return QStringLiteral("F4");
    case KEY_F5: return QStringLiteral("F5");
    case KEY_F6: return QStringLiteral("F6");
    case KEY_F7: return QStringLiteral("F7");
    case KEY_F8: return QStringLiteral("F8");
    case KEY_F9: return QStringLiteral("F9");
    case KEY_F10: return QStringLiteral("F10");
    case KEY_F11: return QStringLiteral("F11");
    case KEY_F12: return QStringLiteral("F12");
    case KEY_KP0: return QStringLiteral("0");
    case KEY_KP1: return QStringLiteral("1");
    case KEY_KP2: return QStringLiteral("2");
    case KEY_KP3: return QStringLiteral("3");
    case KEY_KP4: return QStringLiteral("4");
    case KEY_KP5: return QStringLiteral("5");
    case KEY_KP6: return QStringLiteral("6");
    case KEY_KP7: return QStringLiteral("7");
    case KEY_KP8: return QStringLiteral("8");
    case KEY_KP9: return QStringLiteral("9");
    case KEY_KPDOT: return QStringLiteral(".");
    case KEY_KPENTER: return QStringLiteral("Ent");
    case KEY_KPPLUS: return QStringLiteral("+");
    case KEY_KPMINUS: return QStringLiteral("-");
    case KEY_KPSLASH: return QStringLiteral("/");
    case KEY_KPASTERISK: return QStringLiteral("*");
    default: return {};
    }
}

bool KeyboardInputListener::isNumpadCode(unsigned int code)
{
    return (code >= KEY_KP7 && code <= KEY_KP0) || code == KEY_KPDOT ||
           code == KEY_KPENTER || code == KEY_KPPLUS || code == KEY_KPMINUS ||
           code == KEY_KPSLASH || code == KEY_KPASTERISK;
}

bool KeyboardInputListener::startDevice(const QString &path, QString *error)
{
    if (isRunning()) {
        if (error) *error = QStringLiteral("Stop the current keyboard listener before selecting another device.");
        return false;
    }
    if (!QFileInfo(path).fileName().startsWith(QStringLiteral("event")) ||
        QFileInfo(path).path() != QStringLiteral("/dev/input")) {
        if (error) *error = QStringLiteral("Only a listed /dev/input/event keyboard device can be monitored.");
        return false;
    }
    m_path = path;
    m_stopRequested.store(false);
    start();
    if (error) error->clear();
    return true;
}

void KeyboardInputListener::stop()
{
    m_stopRequested.store(true);
    if (isRunning()) wait(500);
}

void KeyboardInputListener::run()
{
    const QByteArray nativePath = QFile::encodeName(m_path);
    const int fd = open(nativePath.constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        emit listenerError(QStringLiteral("Cannot read %1: %2")
                               .arg(m_path, QString::fromLocal8Bit(std::strerror(errno))));
        emit listenerStopped();
        return;
    }

    while (!m_stopRequested.load()) {
        pollfd descriptor{fd, POLLIN, 0};
        const int ready = poll(&descriptor, 1, 100);
        if (ready < 0) {
            if (errno == EINTR) continue;
            emit listenerError(QStringLiteral("Keyboard input listener failed: %1")
                                   .arg(QString::fromLocal8Bit(std::strerror(errno))));
            break;
        }
        if (ready == 0) continue;
        if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            emit listenerError(QStringLiteral("Keyboard input device disconnected."));
            break;
        }
        if (!(descriptor.revents & POLLIN)) continue;

        input_event events[32];
        const ssize_t bytes = read(fd, events, sizeof(events));
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EINTR) continue;
            emit listenerError(QStringLiteral("Cannot read keyboard events: %1")
                                   .arg(QString::fromLocal8Bit(std::strerror(errno))));
            break;
        }
        const qsizetype count = bytes / static_cast<ssize_t>(sizeof(input_event));
        for (qsizetype i = 0; i < count; ++i) {
            if (events[i].type == EV_KEY && events[i].value == 1) {
                const qint64 timestamp = static_cast<qint64>(events[i].time.tv_sec) * 1000 +
                                         events[i].time.tv_usec / 1000;
                emit keyPressed(events[i].code, timestamp);
            }
        }
    }

    close(fd);
    emit listenerStopped();
}
