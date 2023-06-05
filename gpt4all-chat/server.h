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

    void setPortNumber(int port); // Step 1: Add a setter function for the port number

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void requestServerNewPromptResponsePair(const QString &prompt);

private Q_SLOTS:
    QHttpServerResponse handleCompletionRequest(const QHttpServerRequest &request, bool isChat);

private:
    Chat *m_chat;
    QHttpServer *m_server;
    int m_portNumber; // Step 2: Add a member variable to store the port number
};

#endif // SERVER_H

