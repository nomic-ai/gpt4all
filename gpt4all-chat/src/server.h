#ifndef SERVER_H
#define SERVER_H

#include "chatllm.h"
#include "database.h"

#include <QHttpServerResponse>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>

#include <optional>
#include <utility>

class Chat;
class ChatRequest;
class CompletionRequest;
class QHttpServer;


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

private:
    auto handleCompletionRequest(const CompletionRequest &request) -> std::pair<QHttpServerResponse, std::optional<QJsonObject>>;
    auto handleChatRequest(const ChatRequest &request) -> std::pair<QHttpServerResponse, std::optional<QJsonObject>>;

private Q_SLOTS:
    void handleDatabaseResultsChanged(const QList<ResultInfo> &results) { m_databaseResults = results; }
    void handleCollectionListChanged(const QList<QString> &collectionList) { m_collections = collectionList; }

private:
    Chat *m_chat;
    QHttpServer *m_server;
    QList<ResultInfo> m_databaseResults;
    QList<QString> m_collections;
};

#endif // SERVER_H
