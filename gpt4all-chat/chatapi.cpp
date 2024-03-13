#include "chatapi.h"

#include <string>
#include <vector>
#include <iostream>

#include <QCoreApplication>
#include <QThread>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

//#define DEBUG

ChatAPI::ChatAPI()
    : QObject(nullptr)
    , m_modelName("gpt-3.5-turbo")
    , m_requestURL("")
    , m_responseCallback(nullptr)
{
}

size_t ChatAPI::requiredMem(const std::string &modelPath, int n_ctx, int ngl)
{
    Q_UNUSED(modelPath);
    Q_UNUSED(n_ctx);
    Q_UNUSED(ngl);
    return 0;
}

bool ChatAPI::loadModel(const std::string &modelPath, int n_ctx, int ngl)
{
    Q_UNUSED(modelPath);
    Q_UNUSED(n_ctx);
    Q_UNUSED(ngl);
    return true;
}

void ChatAPI::setThreadCount(int32_t n_threads)
{
    Q_UNUSED(n_threads);
    qt_noop();
}

int32_t ChatAPI::threadCount() const
{
    return 1;
}

ChatAPI::~ChatAPI()
{
}

bool ChatAPI::isModelLoaded() const
{
    return true;
}

// All three of the state virtual functions are handled custom inside of chatllm save/restore
size_t ChatAPI::stateSize() const
{
    return 0;
}

size_t ChatAPI::saveState(uint8_t *dest) const
{
    Q_UNUSED(dest);
    return 0;
}

size_t ChatAPI::restoreState(const uint8_t *src)
{
    Q_UNUSED(src);
    return 0;
}

void ChatAPI::prompt(const std::string &prompt,
                     const std::string &promptTemplate,
                     std::function<bool(int32_t)> promptCallback,
                     std::function<bool(int32_t, const std::string&)> responseCallback,
                     std::function<bool(bool)> recalculateCallback,
                     PromptContext &promptCtx,
                     bool special,
                     std::string *fakeReply) {

    Q_UNUSED(promptCallback);
    Q_UNUSED(recalculateCallback);
    Q_UNUSED(special);

    if (!isModelLoaded()) {
        std::cerr << "ChatAPI ERROR: prompt won't work with an unloaded model!\n";
        return;
    }

    if (!promptCtx.n_past) { m_queuedPrompts.clear(); }
    Q_ASSERT(promptCtx.n_past <= m_context.size());
    m_context.resize(promptCtx.n_past);

    // FIXME(cebtenzzre): We're assuming people don't try to use %2 with ChatGPT. What would that even mean?
    m_queuedPrompts << QString::fromStdString(promptTemplate).arg(QString::fromStdString(prompt));

    if (!promptCtx.n_predict && !fakeReply) {
        return; // response explicitly suppressed, queue prompt for later
    }

    QString formattedPrompt = m_queuedPrompts.join("");
    m_queuedPrompts.clear();

    if (fakeReply) {
        promptCtx.n_past += 1;
        m_context.append(formattedPrompt);
        m_context.append(QString::fromStdString(*fakeReply));
        return;
    }

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

    // conversation history
    QJsonArray messages;
    for (int i = 0; i < m_context.count(); ++i) {
        QJsonObject message;
        message.insert("role", i % 2 == 0 ? "user" : "assistant");
        message.insert("content", m_context.at(i));
        messages.append(message);
    }

    QJsonObject promptObject;
    promptObject.insert("role", "user");
    promptObject.insert("content", formattedPrompt);
    messages.append(promptObject);
    root.insert("messages", messages);

    QJsonDocument doc(root);

#if defined(DEBUG)
    qDebug().noquote() << "ChatAPI::prompt begin network request" << doc.toJson();
#endif

    m_responseCallback = responseCallback;

    // The following code sets up a worker thread and object to perform the actual api request to
    // chatgpt and then blocks until it is finished
    QThread workerThread;
    ChatAPIWorker worker(this);
    worker.moveToThread(&workerThread);
    connect(&worker, &ChatAPIWorker::finished, &workerThread, &QThread::quit, Qt::DirectConnection);
    connect(this, &ChatAPI::request, &worker, &ChatAPIWorker::request, Qt::QueuedConnection);
    workerThread.start();
    emit request(m_apiKey, &promptCtx, doc.toJson(QJsonDocument::Compact));
    workerThread.wait();

    promptCtx.n_past += 1;
    m_context.append(formattedPrompt);
    m_context.append(worker.currentResponse());
    m_responseCallback = nullptr;

#if defined(DEBUG)
    qDebug() << "ChatAPI::prompt end network request";
#endif
}

bool ChatAPI::callResponse(int32_t token, const std::string& string)
{
    Q_ASSERT(m_responseCallback);
    if (!m_responseCallback) {
        std::cerr << "ChatAPI ERROR: no response callback!\n";
        return false;
    }
    return m_responseCallback(token, string);
}

void ChatAPIWorker::request(const QString &apiKey,
                            LLModel::PromptContext *promptCtx,
                            const QByteArray &array)
{
    m_ctx = promptCtx;

    QUrl apiUrl(m_chat->url());
    const QString authorization = QString("Bearer %1").arg(apiKey).trimmed();
    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", authorization.toUtf8());
    m_networkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = m_networkManager->post(request, array);
    connect(qApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &ChatAPIWorker::handleFinished);
    connect(reply, &QNetworkReply::readyRead, this, &ChatAPIWorker::handleReadyRead);
    connect(reply, &QNetworkReply::errorOccurred, this, &ChatAPIWorker::handleErrorOccurred);
}

void ChatAPIWorker::handleFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        emit finished();
        return;
    }

    QVariant response = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    Q_ASSERT(response.isValid());
    bool ok;
    int code = response.toInt(&ok);
    if (!ok || code != 200) {
        qWarning().noquote() << "ERROR: ChatAPIWorker::handleFinished got HTTP Error" << code << "response:"
                             << reply->errorString();
    }
    reply->deleteLater();
    emit finished();
}

void ChatAPIWorker::handleReadyRead()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        emit finished();
        return;
    }

    QVariant response = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    Q_ASSERT(response.isValid());
    bool ok;
    int code = response.toInt(&ok);
    if (!ok || code != 200) {
        m_chat->callResponse(
            -1,
            QString("ERROR: ChatAPIWorker::handleReadyRead got HTTP Error %1 %2: %3")
                .arg(code).arg(reply->errorString()).arg(reply->readAll()).toStdString()
        );
        emit finished();
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
        qDebug().noquote() << "line" << jsonData;
#endif
        QJsonParseError err;
        const QJsonDocument document = QJsonDocument::fromJson(jsonData.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            m_chat->callResponse(-1, QString("ERROR: ChatAPI responded with invalid json \"%1\"")
                                         .arg(err.errorString()).toStdString());
            continue;
        }

        const QJsonObject root = document.object();
        const QJsonArray choices = root.value("choices").toArray();
        const QJsonObject choice = choices.first().toObject();
        const QJsonObject delta = choice.value("delta").toObject();
        const QString content = delta.value("content").toString();
        Q_ASSERT(m_ctx);
        m_currentResponse += content;
        if (!m_chat->callResponse(0, content.toStdString())) {
            reply->abort();
            emit finished();
            return;
        }
    }
}

void ChatAPIWorker::handleErrorOccurred(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply || reply->error() == QNetworkReply::OperationCanceledError /*when we call abort on purpose*/) {
        emit finished();
        return;
    }

    qWarning().noquote() << "ERROR: ChatAPIWorker::handleErrorOccurred got HTTP Error" << code << "response:"
                         << reply->errorString();
    emit finished();
}
