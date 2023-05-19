#include "chatgpt.h"

#include <string>
#include <vector>
#include <iostream>

#include <QThread>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

//#define DEBUG

ChatGPT::ChatGPT()
    : QObject(nullptr)
    , m_modelName("gpt-3.5-turbo")
    , m_ctx(nullptr)
    , m_responseCallback(nullptr)
{
}

bool ChatGPT::loadModel(const std::string &modelPath)
{
    Q_UNUSED(modelPath);
    return true;
}

void ChatGPT::setThreadCount(int32_t n_threads)
{
    Q_UNUSED(n_threads);
    qt_noop();
}

int32_t ChatGPT::threadCount()
{
    return 1;
}

ChatGPT::~ChatGPT()
{
}

bool ChatGPT::isModelLoaded() const
{
    return true;
}

// All three of the state virtual functions are handled custom inside of chatllm save/restore
size_t ChatGPT::stateSize() const
{
    return 0;
}

size_t ChatGPT::saveState(uint8_t *dest) const
{
    Q_UNUSED(dest);
    return 0;
}

size_t ChatGPT::restoreState(const uint8_t *src)
{
    Q_UNUSED(src);
    return 0;
}

void ChatGPT::prompt(const std::string &prompt,
        std::function<bool(int32_t)> promptCallback,
        std::function<bool(int32_t, const std::string&)> responseCallback,
        std::function<bool(bool)> recalculateCallback,
        PromptContext &promptCtx) {

    Q_UNUSED(promptCallback);
    Q_UNUSED(recalculateCallback);

    if (!isModelLoaded()) {
        std::cerr << "ChatGPT ERROR: prompt won't work with an unloaded model!\n";
        return;
    }

    m_ctx = &promptCtx;
    m_responseCallback = responseCallback;

    // FIXME: We don't set the max_tokens on purpose because in order to do so safely without encountering
    // an error we need to be able to count the tokens in our prompt. The only way to do this is to use
    // the OpenAI tiktokken library or to implement our own tokenization function that matches precisely
    // the tokenization used by the OpenAI model we're calling. OpenAI has not introduced any means of
    // using the REST API to count tokens in a prompt.
    QJsonObject root;
    root.insert("model", m_modelName);
    root.insert("stream", true);
    root.insert("temperature", promptCtx.temp);
    root.insert("top_p", promptCtx.top_p);

    QJsonArray messages;
    for (int i = 0; i < m_context.count() && i < promptCtx.n_past; ++i) {
        QJsonObject message;
        message.insert("role", i % 2 == 0 ? "assistant" : "user");
        message.insert("content", m_context.at(i));
        messages.append(message);
    }

    QJsonObject promptObject;
    promptObject.insert("role", "user");
    promptObject.insert("content", QString::fromStdString(prompt));
    messages.append(promptObject);
    root.insert("messages", messages);

    QJsonDocument doc(root);

#if defined(DEBUG)
    qDebug() << "ChatGPT::prompt begin network request" << qPrintable(doc.toJson());
#endif

    QEventLoop loop;
    QUrl openaiUrl("https://api.openai.com/v1/chat/completions");
    const QString authorization = QString("Bearer %1").arg(m_apiKey);
    QNetworkRequest request(openaiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", authorization.toUtf8());
    QNetworkReply *reply = m_networkManager.post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, this, &ChatGPT::handleFinished);
    connect(reply, &QNetworkReply::readyRead, this, &ChatGPT::handleReadyRead);
    connect(reply, &QNetworkReply::errorOccurred, this, &ChatGPT::handleErrorOccurred);
    loop.exec();
#if defined(DEBUG)
    qDebug() << "ChatGPT::prompt end network request";
#endif

    m_ctx->n_past += 1;
    m_context.append(QString::fromStdString(prompt));
    m_context.append(m_currentResponse);

    m_ctx = nullptr;
    m_responseCallback = nullptr;
    m_currentResponse = QString();
}

void ChatGPT::handleFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    QVariant response = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    Q_ASSERT(response.isValid());
    bool ok;
    int code = response.toInt(&ok);
    if (!ok || code != 200) {
        qWarning() << QString("ERROR: ChatGPT responded with error code \"%1-%2\"")
            .arg(code).arg(reply->errorString()).toStdString();
    }
    reply->deleteLater();
}

void ChatGPT::handleReadyRead()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    QVariant response = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    Q_ASSERT(response.isValid());
    bool ok;
    int code = response.toInt(&ok);
    if (!ok || code != 200) {
        m_responseCallback(-1, QString("\nERROR: 2 ChatGPT responded with error code \"%1-%2\" %3\n")
            .arg(code).arg(reply->errorString()).arg(qPrintable(reply->readAll())).toStdString());
        return;
    }

    while (reply->canReadLine()) {
        QString jsonData = reply->readLine().trimmed();
        if (jsonData.startsWith("data:"))
            jsonData.remove(0, 5);
        jsonData = jsonData.trimmed();
        if (jsonData.isEmpty())
            continue;
        if (jsonData == "[DONE]")
            continue;
#if defined(DEBUG)
        qDebug() << "line" << qPrintable(jsonData);
#endif
        QJsonParseError err;
        const QJsonDocument document = QJsonDocument::fromJson(jsonData.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            m_responseCallback(-1, QString("\nERROR: ChatGPT responded with invalid json \"%1\"\n")
                .arg(err.errorString()).toStdString());
            continue;
        }

        const QJsonObject root = document.object();
        const QJsonArray choices = root.value("choices").toArray();
        const QJsonObject choice = choices.first().toObject();
        const QJsonObject delta = choice.value("delta").toObject();
        const QString content = delta.value("content").toString();
        Q_ASSERT(m_ctx);
        Q_ASSERT(m_responseCallback);
        m_currentResponse += content;
        if (!m_responseCallback(0, content.toStdString())) {
            reply->abort();
            return;
        }
    }
}

void ChatGPT::handleErrorOccurred(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    qWarning() << QString("ERROR: ChatGPT responded with error code \"%1-%2\"")
                      .arg(code).arg(reply->errorString()).toStdString();
}
