#ifndef SERVER_H
#define SERVER_H

#include "chatllm.h"

#include <QObject>
#include <QtHttpServer/QHttpServer>

class Server : public ChatLLM
{
    Q_OBJECT

public:
    Server(Chat *parent);
    virtual ~Server();

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void requestServerNewPromptResponsePair(const QString &prompt);

private Q_SLOTS:
    QHttpServerResponse handleCompletionRequest(const QHttpServerRequest &request, bool isChat);
    void handleDatabaseResultsChanged(const QList<ResultInfo> &results) { m_databaseResults = results; }
    void handleCollectionListChanged(const QList<QString> &collectionList) { m_collections = collectionList; }

private:
    Chat *m_chat;
    QHttpServer *m_server;
    QList<ResultInfo> m_databaseResults;
    QList<QString> m_collections;
};

#endif // SERVER_H
