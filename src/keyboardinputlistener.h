#pragma once

#include <QThread>
#include <QStringList>
#include <atomic>

struct KeyboardInputDevice {
    QString path;
    QString name;
};

class KeyboardInputListener final : public QThread {
    Q_OBJECT
public:
    explicit KeyboardInputListener(QObject *parent = nullptr);
    ~KeyboardInputListener() override;

    static QList<KeyboardInputDevice> availableKeyboards();
    static QString keyLabelForCode(unsigned int code);
    static bool isNumpadCode(unsigned int code);
    bool startDevice(const QString &path, QString *error = nullptr);
    void stop();

signals:
    void keyPressed(unsigned int code, qint64 timestampMs);
    void listenerError(const QString &message);
    void listenerStopped();

protected:
    void run() override;

private:
    QString m_path;
    std::atomic_bool m_stopRequested{false};
};
