#ifndef LLM_H
#define LLM_H

#include <QObject>

#include "chat.h"

class LLM : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QString> modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(bool isRecalc READ isRecalc NOTIFY recalcChanged)
    Q_PROPERTY(Chat *currentChat READ currentChat NOTIFY currentChatChanged)
    Q_PROPERTY(QList<QString> chatList READ chatList NOTIFY chatListChanged)

public:

    static LLM *globalInstance();

    QList<QString> modelList() const;
    bool isRecalc() const;
    Chat *currentChat() const;
    QList<QString> chatList() const;

    Q_INVOKABLE QString addChat();
    Q_INVOKABLE void removeChat(const QString &id);
    Q_INVOKABLE Chat *chatFromId(const QString &id) const;
    Q_INVOKABLE void setCurrentChatFromId(const QString &id);
    Q_INVOKABLE bool checkForUpdates() const;

Q_SIGNALS:
    void modelListChanged();
    void currentChatChanged();
    void recalcChanged();
    void chatListChanged();
    void responseChanged();

private:
    void connectChat(Chat *chat);
    void disconnectChat(Chat *chat);

private:
    QString m_currentChat;
    QMap<QString, Chat*> m_chats;

private:
    explicit LLM();
    ~LLM() {}
    friend class MyLLM;
};

#endif // LLM_H
