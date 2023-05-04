#ifndef LLM_H
#define LLM_H

#include <QObject>

#include "chatlistmodel.h"

class LLM : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ChatListModel *chatListModel READ chatListModel NOTIFY chatListModelChanged)
    Q_PROPERTY(int32_t threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)

public:
    static LLM *globalInstance();

    ChatListModel *chatListModel() const { return m_chatListModel; }
    int32_t threadCount() const;
    void setThreadCount(int32_t n_threads);

    Q_INVOKABLE bool checkForUpdates() const;

Q_SIGNALS:
    void chatListModelChanged();
    void threadCountChanged();

private Q_SLOTS:
    void aboutToQuit();

private:
    ChatListModel *m_chatListModel;
    int32_t m_threadCount;

private:
    explicit LLM();
    ~LLM() {}
    friend class MyLLM;
};

#endif // LLM_H
