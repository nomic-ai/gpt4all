#include "server.h"
#include "llm.h"
#include "download.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <iostream>

//#define DEBUG

static inline QString modelToName(const ModelInfo &info)
{
    QString modelName = info.filename;
    Q_ASSERT(modelName.startsWith("ggml-"));
    modelName = modelName.remove(0, 5);
    Q_ASSERT(modelName.endsWith(".bin"));
    modelName.chop(4);
    return modelName;
}

static inline QJsonObject modelToJson(const ModelInfo &info)
{
    QString modelName = modelToName(info);

    QJsonObject model;
    model.insert("id", modelName);
    model.insert("object", "model");
    model.insert("created", "who can keep track?");
    model.insert("owned_by", "humanity");
    model.insert("root", modelName);
    model.insert("parent", QJsonValue::Null);

    QJsonArray permissions;
    QJsonObject permissionObj;
    permissionObj.insert("id", "foobarbaz");
    permissionObj.insert("object", "model_permission");
    permissionObj.insert("created", "does it really matter?");
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

Server::Server(Chat *chat)
    : ChatLLM(chat, true /*isServer*/)
    , m_chat(chat)
    , m_server(nullptr)
{
    connect(this, &Server::threadStarted, this, &Server::start);
}

Server::~Server()
{
}

void Server::start()
{
    m_server = new QHttpServer(this);
    if (!m_server->listen(QHostAddress::LocalHost, 4891)) {
        qWarning() << "ERROR: Unable to start the server";
        return;
    }

    m_server->route("/v1/models", QHttpServerRequest::Method::Get,
        [](const QHttpServerRequest &request) {
            if (!LLM::globalInstance()->serverEnabled())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);

            const QList<ModelInfo> modelList = Download::globalInstance()->modelList();
            QJsonObject root;
            root.insert("object", "list");
            QJsonArray data;
            for (const ModelInfo &info : modelList) {
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
            if (!LLM::globalInstance()->serverEnabled())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);

            const QList<ModelInfo> modelList = Download::globalInstance()->modelList();
            QJsonObject object;
            for (const ModelInfo &info : modelList) {
                if (!info.installed)
                    continue;

                QString modelName = modelToName(info);
                if (model == modelName) {
                    object = modelToJson(info);
                    break;
                }
            }
            return QHttpServerResponse(object);
        }
    );

    m_server->route("/v1/completions", QHttpServerRequest::Method::Post,
        [=](const QHttpServerRequest &request) {
            if (!LLM::globalInstance()->serverEnabled())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
            return handleCompletionRequest(request, false);
        }
    );

    m_server->route("/v1/chat/completions", QHttpServerRequest::Method::Post,
        [=](const QHttpServerRequest &request) {
            if (!LLM::globalInstance()->serverEnabled())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
            return handleCompletionRequest(request, true);
        }
    );

    connect(this, &Server::requestServerNewPromptResponsePair, m_chat,
        &Chat::serverNewPromptResponsePair, Qt::BlockingQueuedConnection);
}

QHttpServerResponse Server::handleCompletionRequest(const QHttpServerRequest &request, bool isChat)
{
    // We've been asked to do a completion...
    QJsonParseError err;
    const QJsonDocument document = QJsonDocument::fromJson(request.body(), &err);
    if (err.error || !document.isObject()) {
        std::cerr << "ERROR: invalid json in completions body" << std::endl;
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    }
#if defined(DEBUG)
    printf("/v1/completions %s\n", qPrintable(document.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif
    const QJsonObject body = document.object();
    if (!body.contains("model")) { // required
        std::cerr << "ERROR: completions contains no model" << std::endl;
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    }
    QJsonArray messages;
    if (isChat) {
        if (!body.contains("messages")) {
            std::cerr << "ERROR: chat completions contains no messages" << std::endl;
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
        }
        messages = body["messages"].toArray();
    }

    const QString model = body["model"].toString();
    bool foundModel = false;
    const QList<ModelInfo> modelList = Download::globalInstance()->modelList();
    for (const ModelInfo &info : modelList) {
        if (!info.installed)
            continue;
        if (model == modelToName(info)) {
            foundModel = true;
            break;
        }
    }

    // We only support one prompt for now
    QList<QString> prompts;
    if (body.contains("prompt")) {
        QJsonValue promptValue = body["prompt"];
        if (promptValue.isString())
            prompts.append(promptValue.toString());
        else {
            QJsonArray array = promptValue.toArray();
            for (QJsonValue v : array)
                prompts.append(v.toString());
        }
    } else
        prompts.append(" ");

    int max_tokens = 16;
    if (body.contains("max_tokens"))
        max_tokens = body["max_tokens"].toInt();

    float temperature = 1.f;
    if (body.contains("temperature"))
        temperature = body["temperature"].toDouble();

    float top_p = 1.f;
    if (body.contains("top_p"))
        top_p = body["top_p"].toDouble();

    int n = 1;
    if (body.contains("n"))
        n = body["n"].toInt();

    int logprobs = -1; // supposed to be null by default??
    if (body.contains("logprobs"))
        logprobs = body["logprobs"].toInt();

    bool echo = false;
    if (body.contains("echo"))
        echo = body["echo"].toBool();

    // We currently don't support any of the following...
#if 0
    // FIXME: Need configurable reverse prompts
    QList<QString> stop;
    if (body.contains("stop")) {
        QJsonValue stopValue = body["stop"];
        if (stopValue.isString())
            stop.append(stopValue.toString());
        else {
            QJsonArray array = stopValue.toArray();
            for (QJsonValue v : array)
                stop.append(v.toString());
        }
    }

    // FIXME: QHttpServer doesn't support server-sent events
    bool stream = false;
    if (body.contains("stream"))
        stream = body["stream"].toBool();

    // FIXME: What does this do?
    QString suffix;
    if (body.contains("suffix"))
        suffix = body["suffix"].toString();

    // FIXME: We don't support
    float presence_penalty = 0.f;
    if (body.contains("presence_penalty"))
        top_p = body["presence_penalty"].toDouble();

    // FIXME: We don't support
    float frequency_penalty = 0.f;
    if (body.contains("frequency_penalty"))
        top_p = body["frequency_penalty"].toDouble();

    // FIXME: We don't support
    int best_of = 1;
    if (body.contains("best_of"))
        logprobs = body["best_of"].toInt();

    // FIXME: We don't need
    QString user;
    if (body.contains("user"))
        suffix = body["user"].toString();
#endif

    QString actualPrompt = prompts.first();

    // if we're a chat completion we have messages which means we need to prepend these to the prompt
    if (!messages.isEmpty()) {
        QList<QString> chats;
        for (int i = 0; i < messages.count();  ++i) {
            QJsonValue v = messages.at(i);
            QString content = v.toObject()["content"].toString();
            if (!content.endsWith("\n") && i < messages.count() - 1)
                content += "\n";
            chats.append(content);
        }
        actualPrompt.prepend(chats.join("\n"));
    }

    // adds prompt/response items to GUI
    emit requestServerNewPromptResponsePair(actualPrompt); // blocks

    // load the new model if necessary
    setShouldBeLoaded(true);

    if (!foundModel) {
        if (!loadDefaultModel()) {
            std::cerr << "ERROR: couldn't load default model " << model.toStdString() << std::endl;
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }
    } else if (!loadModel(model)) {
        std::cerr << "ERROR: couldn't load model " << model.toStdString() << std::endl;
        return QHttpServerResponse(QHttpServerResponder::StatusCode::InternalServerError);
    }

    // don't remember any context
    resetContextProtected();

    QSettings settings;
    settings.sync();
    const QString promptTemplate = settings.value("promptTemplate", "%1").toString();
    const float top_k = settings.value("topK", m_ctx.top_k).toDouble();
    const int n_batch = settings.value("promptBatchSize", m_ctx.n_batch).toInt();
    const float repeat_penalty = settings.value("repeatPenalty", m_ctx.repeat_penalty).toDouble();
    const int repeat_last_n = settings.value("repeatPenaltyTokens", m_ctx.repeat_last_n).toInt();

    int promptTokens = 0;
    int responseTokens = 0;
    QList<QString> responses;
    for (int i = 0; i < n; ++i) {
        if (!prompt(actualPrompt,
            promptTemplate,
            max_tokens /*n_predict*/,
            top_k,
            top_p,
            temperature,
            n_batch,
            repeat_penalty,
            repeat_last_n,
            LLM::globalInstance()->threadCount())) {

            std::cerr << "ERROR: couldn't prompt model " << model.toStdString() << std::endl;
            return QHttpServerResponse(QHttpServerResponder::StatusCode::InternalServerError);
        }
        QString echoedPrompt = actualPrompt;
        if (!echoedPrompt.endsWith("\n"))
            echoedPrompt += "\n";
        responses.append((echo ? QString("%1\n").arg(actualPrompt) : QString()) + response());
        if (!promptTokens)
            promptTokens += m_promptTokens;
        responseTokens += m_promptResponseTokens - m_promptTokens;
        if (i != n - 1)
            resetResponse();
    }

    QJsonObject responseObject;
    responseObject.insert("id", "foobarbaz");
    responseObject.insert("object", "text_completion");
    responseObject.insert("created", QDateTime::currentSecsSinceEpoch());
    responseObject.insert("model", modelName());

    QJsonArray choices;
    int index = 0;
    for (QString r : responses) {
        QJsonObject choice;
        choice.insert("text", r);
        choice.insert("index", index++);
        choice.insert("logprobs", QJsonValue::Null); // We don't support
        choice.insert("finish_reason", responseTokens == max_tokens ? "length" : "stop");
        choices.append(choice);
    }
    responseObject.insert("choices", choices);

    QJsonObject usage;
    usage.insert("prompt_tokens", int(promptTokens));
    usage.insert("completion_tokens", int(responseTokens));
    usage.insert("total_tokens", int(promptTokens + responseTokens));
    responseObject.insert("usage", usage);

#if defined(DEBUG)
    QJsonDocument newDoc(responseObject);
    printf("/v1/completions %s\n", qPrintable(newDoc.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif

    return QHttpServerResponse(responseObject);
}
