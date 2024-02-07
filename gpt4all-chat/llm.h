#ifndef LLM_H
#define LLM_H

#include <QObject>

class LLM : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isNetworkOnline READ isNetworkOnline NOTIFY isNetworkOnlineChanged)

public:
    static LLM *globalInstance();

    Q_INVOKABLE bool hasSettingsAccess() const;
    Q_INVOKABLE bool compatHardware() const { return m_compatHardware; }

    Q_INVOKABLE bool checkForUpdates() const;
    Q_INVOKABLE static bool directoryExists(const QString &path);
    Q_INVOKABLE static bool fileExists(const QString &path);
    Q_INVOKABLE qint64 systemTotalRAMInGB() const;
    Q_INVOKABLE QString systemTotalRAMInGBString() const;
    Q_INVOKABLE bool isNetworkOnline() const;

Q_SIGNALS:
    void isNetworkOnlineChanged();

private:
    bool m_compatHardware;

private:
    explicit LLM();
    ~LLM() {}
    friend class MyLLM;
};

#endif // LLM_H
