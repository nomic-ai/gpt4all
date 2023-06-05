#ifndef SERVER_H
#define SERVER_H

#include "chatllm.h"

#include <QObject>
#include <QtHttpServer/QHttpServer>
#include <QSettings> // Include the QSettings header

class Server : public ChatLLM
{
    Q_OBJECT

public:
    Server(Chat *parent);
    virtual ~Server();

    void setPortNumber(int port);

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void requestServerNewPromptResponsePair(const QString &prompt);

private Q_SLOTS:
    QHttpServerResponse handleCompletionRequest(const QHttpServerRequest &request, bool isChat);

private:
    Chat *m_chat;
    QHttpServer *m_server;
    int m_portNumber;

    // Step 3: Add a member variable to store the QSettings object
    QSettings *m_settings;
};

#endif // SERVER_H
