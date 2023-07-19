#ifndef LLM_H
#define LLM_H

#include <QObject>

class LLM : public QObject
{
    Q_OBJECT
public:
    static LLM *globalInstance();

    Q_INVOKABLE bool hasSettingsAccess() const;
    Q_INVOKABLE bool compatHardware() const { return m_compatHardware; }

    Q_INVOKABLE bool checkForUpdates() const;
    Q_INVOKABLE bool directoryExists(const QString &path) const;
    Q_INVOKABLE bool fileExists(const QString &path) const;
    Q_INVOKABLE qint64 systemTotalRAMInGB() const;
    Q_INVOKABLE QString systemTotalRAMInGBString() const;

Q_SIGNALS:
    void chatListModelChanged();
    void modelListChanged();

private:
    bool m_compatHardware;

private:
    explicit LLM();
    ~LLM() {}
    friend class MyLLM;
};

#endif // LLM_H
