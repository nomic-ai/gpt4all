#ifndef LLM_H
#define LLM_H

#include <QObject>

#include "chat.h"

class LLM : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QString> modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(Chat *currentChat READ currentChat NOTIFY currentChatChanged)
    Q_PROPERTY(bool isRecalc READ isRecalc NOTIFY recalcChanged)

public:

    static LLM *globalInstance();

    QList<QString> modelList() const;
    Q_INVOKABLE bool checkForUpdates() const;
    Chat *currentChat() const { return m_currentChat; }
    bool isRecalc() const;

Q_SIGNALS:
    void modelListChanged();
    void currentChatChanged();
    void recalcChanged();
    void responseChanged();

private:
    Chat *m_currentChat;

private:
    explicit LLM();
    ~LLM() {}
    friend class MyLLM;
};

#endif // LLM_H
