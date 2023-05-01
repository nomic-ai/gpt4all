#ifndef LLM_H
#define LLM_H

#include <QObject>

#include "chat.h"
#include "chatlistmodel.h"

class LLM : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QString> modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(bool isRecalc READ isRecalc NOTIFY recalcChanged)
    Q_PROPERTY(ChatListModel *chatListModel READ chatListModel NOTIFY chatListModelChanged)

public:
    static LLM *globalInstance();

    QList<QString> modelList() const;
    bool isRecalc() const;
    ChatListModel *chatListModel() const { return m_chatListModel; }

    Q_INVOKABLE bool checkForUpdates() const;

Q_SIGNALS:
    void modelListChanged();
    void recalcChanged();
    void responseChanged();
    void chatListModelChanged();

private Q_SLOTS:
    void connectChat(Chat *chat);
    void disconnectChat(Chat *chat);

private:
    ChatListModel *m_chatListModel;

private:
    explicit LLM();
    ~LLM() {}
    friend class MyLLM;
};

#endif // LLM_H
