#include "server.h"

#include "chat.h"
#include "modellist.h"
#include "mysettings.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerResponder>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPair>
#include <Qt>
#include <QtLogging>

#include <iostream>
#include <utility>

using namespace Qt::Literals::StringLiterals;

//#define DEBUG

static inline QJsonObject modelToJson(const ModelInfo &info)
{
    QJsonObject model;
    model.insert("id", info.name());
    model.insert("object", "model");
    model.insert("created", 0);
    model.insert("owned_by", "humanity");
    model.insert("root", info.name());
    model.insert("parent", QJsonValue::Null);

    QJsonArray permissions;
    QJsonObject permissionObj;
    permissionObj.insert("id", "foobarbaz");
    permissionObj.insert("object", "model_permission");
    permissionObj.insert("created", 0);
    permissionObj.insert("allow_create_engine", false);
    permissionObj.insert("allow_sampling", false);
    permissionObj.insert("allow_logprobs", false);
    permissionObj.insert("allow_search_indices", false);
    permissionObj.insert("allow_view", true);
    permissionObj.insert("allow_fine_tuning", false);
    permissionObj.insert("organization", "*");
    permissionObj.insert("group", QJsonValue::Null);
    permissionObj.insert("is_blocking", false);
    permissions.append(permissionObj);
    model.insert("permissions", permissions);
    return model;
}

static inline QJsonObject resultToJson(const ResultInfo &info)
{
    QJsonObject result;
    result.insert("file", info.file);
    result.insert("title", info.title);
    result.insert("author", info.author);
    result.insert("date", info.date);
    result.insert("text", info.text);
    result.insert("page", info.page);
    result.insert("from", info.from);
    result.insert("to", info.to);
    return result;
}

Server::Server(Chat *chat)
    : ChatLLM(chat, true /*isServer*/)
    , m_chat(chat)
    , m_server(nullptr)
{
    connect(this, &Server::threadStarted, this, &Server::start);
    connect(this, &Server::databaseResultsChanged, this, &Server::handleDatabaseResultsChanged);
    connect(chat, &Chat::collectionListChanged, this, &Server::handleCollectionListChanged, Qt::QueuedConnection);
}

Server::~Server()
{
}

void Server::start()
{
    m_server = new QHttpServer(this);
    if (!m_server->listen(QHostAddress::LocalHost, MySettings::globalInstance()->networkPort())) {
        qWarning() << "ERROR: Unable to start the server";
        return;
    }

    m_server->route("/v1/models", QHttpServerRequest::Method::Get,
        [](const QHttpServerRequest &request) {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);

            const QList<ModelInfo> modelList = ModelList::globalInstance()->selectableModelList();
            QJsonObject root;
            root.insert("object", "list");
            QJsonArray data;
            for (const ModelInfo &info : modelList) {
                Q_ASSERT(info.installed);
                if (!info.installed)
                    continue;
                data.append(modelToJson(info));
            }
            root.insert("data", data);
            return QHttpServerResponse(root);
        }
    );

    m_server->route("/v1/models/<arg>", QHttpServerRequest::Method::Get,
        [](const QString &model, const QHttpServerRequest &request) {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);

            const QList<ModelInfo> modelList = ModelList::globalInstance()->selectableModelList();
            QJsonObject object;
            for (const ModelInfo &info : modelList) {
                Q_ASSERT(info.installed);
                if (!info.installed)
                    continue;

                if (model == info.name()) {
                    object = modelToJson(info);
                    break;
                }
            }
            return QHttpServerResponse(object);
        }
    );

    m_server->route("/v1/completions", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
            return handleCompletionRequest(request, false);
        }
    );

    m_server->route("/v1/chat/completions", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
            return handleCompletionRequest(request, true);
        }
    );

    // Respond with code 405 to wrong HTTP methods:
    m_server->route("/v1/models",  QHttpServerRequest::Method::Post,
        [](const QHttpServerRequest &request) {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
            return QHttpServerResponse(
                QJsonDocument::fromJson("{\"error\": {\"message\": \"Not allowed to POST on /v1/models."
                    " (HINT: Perhaps you meant to use a different HTTP method?)\","
                    " \"type\": \"invalid_request_error\", \"param\": null, \"code\": null}}").object(),
                QHttpServerResponder::StatusCode::MethodNotAllowed);
        }
    );

    m_server->route("/v1/models/<arg>", QHttpServerRequest::Method::Post,
        [](const QString &model, const QHttpServerRequest &request) {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
            return QHttpServerResponse(
                QJsonDocument::fromJson("{\"error\": {\"message\": \"Not allowed to POST on /v1/models/*."
                    " (HINT: Perhaps you meant to use a different HTTP method?)\","
                    " \"type\": \"invalid_request_error\", \"param\": null, \"code\": null}}").object(),
                QHttpServerResponder::StatusCode::MethodNotAllowed);
        }
    );

    m_server->route("/v1/completions", QHttpServerRequest::Method::Get,
        [](const QHttpServerRequest &request) {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
            return QHttpServerResponse(
                QJsonDocument::fromJson("{\"error\": {\"message\": \"Only POST requests are accepted.\","
                    " \"type\": \"invalid_request_error\", \"param\": null, \"code\": \"method_not_supported\"}}").object(),
                QHttpServerResponder::StatusCode::MethodNotAllowed);
        }
    );

    m_server->route("/v1/chat/completions", QHttpServerRequest::Method::Get,
        [](const QHttpServerRequest &request) {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
            return QHttpServerResponse(
                QJsonDocument::fromJson("{\"error\": {\"message\": \"Only POST requests are accepted.\","
                    " \"type\": \"invalid_request_error\", \"param\": null, \"code\": \"method_not_supported\"}}").object(),
                QHttpServerResponder::StatusCode::MethodNotAllowed);
        }
    );

    m_server->afterRequest([] (QHttpServerResponse &&resp) {
        resp.addHeader("Access-Control-Allow-Origin", "*");
        return std::move(resp);
    });

    connect(this, &Server::requestServerNewPromptResponsePair, m_chat,
        &Chat::serverNewPromptResponsePair, Qt::BlockingQueuedConnection);
}

QHttpServerResponse Server::handleCompletionRequest(const QHttpServerRequest &request, bool isChat)
{
    // Parse JSON request
    QJsonParseError err;
    const QJsonDocument document = QJsonDocument::fromJson(request.body(), &err);
    if (err.error || !document.isObject()) {
        std::cerr << "ERROR: invalid JSON in completions body" << std::endl;
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    }

#if defined(DEBUG)
    printf("/v1/completions %s\n", qPrintable(document.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif

    const QJsonObject body = document.object();
    if (!body.contains("model")) {
        std::cerr << "ERROR: completions contain no model" << std::endl;
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    }

    QJsonArray messages;
    if (isChat) {
        if (!body.contains("messages")) {
            std::cerr << "ERROR: chat completions contain no messages" << std::endl;
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
        }
        messages = body["messages"].toArray();
    }

    const QString modelRequested = body["model"].toString();
    ModelInfo modelInfo = ModelList::globalInstance()->defaultModelInfo();
    const QList<ModelInfo> modelList = ModelList::globalInstance()->selectableModelList();
    for (const ModelInfo &info : modelList) {
        if (info.installed && (modelRequested == info.name() || modelRequested == info.filename())) {
            modelInfo = info;
            break;
        }
    }

    QList<QString> prompts;
    if (body.contains("prompt")) {
        QJsonValue promptValue = body["prompt"];
        if (promptValue.isString())
            prompts.append(promptValue.toString());
        else {
            QJsonArray array = promptValue.toArray();
            for (const QJsonValue &v : array)
                prompts.append(v.toString());
        }
    } else {
        prompts.append(" ");
    }

    int max_tokens = body.value("max_tokens").toInt(16);
    float temperature = body.value("temperature").toDouble(1.0);
    float top_p = body.value("top_p").toDouble(1.0);
    float min_p = body.value("min_p").toDouble(0.0);
    int n = body.value("n").toInt(1);
    bool echo = body.value("echo").toBool(false);

    QString actualPrompt = prompts.first();

    if (!messages.isEmpty()) {
        QList<QString> chats;
        for (int i = 0; i < messages.count(); ++i) {
            QString content = messages.at(i).toObject()["content"].toString();
            if (!content.endsWith("\n") && i < messages.count() - 1)
                content += "\n";
            chats.append(content);
        }
        actualPrompt.prepend(chats.join("\n"));
    }

    emit requestServerNewPromptResponsePair(actualPrompt); // blocks

    setShouldBeLoaded(true);

    if (modelInfo.filename().isEmpty()) {
        std::cerr << "ERROR: couldn't load default model " << modelRequested.toStdString() << std::endl;
        return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
    } else if (!loadModel(modelInfo)) {
        std::cerr << "ERROR: couldn't load model " << modelInfo.name().toStdString() << std::endl;
        return QHttpServerResponse(QHttpServerResponder::StatusCode::InternalServerError);
    }

    resetContext();

    QByteArray responseData;
    QTextStream stream(&responseData, QIODevice::WriteOnly);

    QString randomId = "chatcmpl-" + QUuid::createUuid().toString(QUuid::WithoutBraces).replace("-", "");

    for (int i = 0; i < n; ++i) {
        if (!promptInternal(m_collections,
                            actualPrompt,
                            modelInfo.promptTemplate(),
                            max_tokens /*n_predict*/,
                            modelInfo.topK(),
                            top_p,
                            min_p,
                            temperature,
                            modelInfo.promptBatchSize(),
                            modelInfo.repeatPenalty(),
                            modelInfo.repeatPenaltyTokens())) {

            std::cerr << "ERROR: couldn't prompt model " << modelInfo.name().toStdString() << std::endl;
            return QHttpServerResponse(QHttpServerResponder::StatusCode::InternalServerError);
        }

        QString result = (echo ? u"%1\n"_s.arg(actualPrompt) : QString()) + response();

        for (const QString &token : result.split(' ')) {
            QJsonObject delta;
            delta.insert("content", token + " ");

            QJsonObject choice;
            choice.insert("index", i);
            choice.insert("delta", delta);

            QJsonObject responseChunk;
            responseChunk.insert("id", randomId);
            responseChunk.insert("object", "chat.completion.chunk");
            responseChunk.insert("created", QDateTime::currentSecsSinceEpoch());
            responseChunk.insert("model", modelInfo.name());
            responseChunk.insert("choices", QJsonArray{choice});

            stream << "data: " << QJsonDocument(responseChunk).toJson(QJsonDocument::Compact) << "\n\n";
            stream.flush();
        }

        if (i != n - 1)
            resetResponse();
    }

    // Final empty delta to signify the end of the stream
    QJsonObject delta;
    delta.insert("content", QJsonValue::Null);

    QJsonObject choice;
    choice.insert("index", 0);
    choice.insert("delta", delta);
    choice.insert("finish_reason", "stop");

    QJsonObject finalChunk;
    finalChunk.insert("id", randomId);
    finalChunk.insert("object", "chat.completion.chunk");
    finalChunk.insert("created", QDateTime::currentSecsSinceEpoch());
    finalChunk.insert("model", modelInfo.name());
    finalChunk.insert("choices", QJsonArray{choice});

    stream << "data: " << QJsonDocument(finalChunk).toJson(QJsonDocument::Compact) << "\n\n";
    stream << "data: [DONE]\n\n";
    stream.flush();

    // Log the entire response data
    qDebug() << "Full SSE Response:\n" << responseData;

    // Create the response
    QHttpServerResponse response(responseData, QHttpServerResponder::StatusCode::Ok);
    response.setHeader("Content-Type", "text/event-stream");
    response.setHeader("Cache-Control", "no-cache");
    response.setHeader("Connection", "keep-alive");

    return response;
}
