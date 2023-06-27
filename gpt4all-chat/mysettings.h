#ifndef MYSETTINGS_H
#define MYSETTINGS_H

#include <QObject>
#include <QMutex>

class MySettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool forceMetal READ forceMetal WRITE setForceMetal NOTIFY forceMetalChanged)

public:
    static MySettings *globalInstance();

    bool forceMetal() const;
    void setForceMetal(bool enabled);

Q_SIGNALS:
    void forceMetalChanged(bool);

private:
    bool m_forceMetal;

private:
    explicit MySettings();
    ~MySettings() {}
    friend class MyPrivateSettings;
};

#endif // MYSETTINGS_H
