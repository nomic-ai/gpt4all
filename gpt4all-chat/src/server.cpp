#include "server.h"

#include "chat.h"
#include "chatmodel.h"
#include "modellist.h"
#include "mysettings.h"
#include "utils.h"

#include <fmt/format.h>

#include <QByteArray>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include <QDateTime>
#include <QDebug>
#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerResponder>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLatin1StringView>
#include <QPair>
#include <QVariant>
#include <Qt>
#include <QtCborCommon>
#include <QtGlobal>
#include <QtLogging>

#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#   include <QTcpServer>
#endif

using namespace std::string_literals;
using namespace Qt::Literals::StringLiterals;

//#define DEBUG


namespace {

class InvalidRequestError: public std::invalid_argument {
    using std::invalid_argument::invalid_argument;

public:
    QHttpServerResponse asResponse() const
    {
        QJsonObject error {
            { "message", what(),                     },
            { "type",    u"invalid_request_error"_s, },
            { "param",   QJsonValue::Null            },
            { "code",    QJsonValue::Null            },
        };
        return { QJsonObject {{ "error", error }},
                 QHttpServerResponder::StatusCode::BadRequest };
    }

private:
    Q_DISABLE_COPY_MOVE(InvalidRequestError)
};

} // namespace

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
    permissionObj.insert("id", "placeholder");
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

class BaseCompletionRequest {
public:
    QString model; // required
    // NB: some parameters are not supported yet
    int32_t max_tokens = 16;
    qint64 n = 1;
    float temperature = 1.f;
    float top_p = 1.f;
    float min_p = 0.f;

    BaseCompletionRequest() = default;
    virtual ~BaseCompletionRequest() = default;

    virtual BaseCompletionRequest &parse(QCborMap request)
    {
        parseImpl(request);
        if (!request.isEmpty())
            throw InvalidRequestError(fmt::format(
                "Unrecognized request argument supplied: {}", request.keys().constFirst().toString()
            ));
        return *this;
    }

protected:
    virtual void parseImpl(QCborMap &request)
    {
        using enum Type;

        auto reqValue = [&request](auto &&...args) { return takeValue(request, args...); };
        QCborValue value;

        this->model = reqValue("model", String, /*required*/ true).toString();

        value = reqValue("frequency_penalty", Number, false, /*min*/ -2, /*max*/ 2);
        if (value.isDouble() || value.toInteger() != 0)
            throw InvalidRequestError("'frequency_penalty' is not supported");

        value = reqValue("max_tokens", Integer, false, /*min*/ 1);
        if (!value.isNull())
            this->max_tokens = int32_t(qMin(value.toInteger(), INT32_MAX));

        value = reqValue("n", Integer, false, /*min*/ 1);
        if (!value.isNull())
            this->n = value.toInteger();

        value = reqValue("presence_penalty", Number);
        if (value.isDouble() || value.toInteger() != 0)
            throw InvalidRequestError("'presence_penalty' is not supported");

        value = reqValue("seed", Integer);
        if (!value.isNull())
            throw InvalidRequestError("'seed' is not supported");

        value = reqValue("stop");
        if (!value.isNull())
            throw InvalidRequestError("'stop' is not supported");

        value = reqValue("stream", Boolean);
        if (value.isTrue())
            throw InvalidRequestError("'stream' is not supported");

        value = reqValue("stream_options", Object);
        if (!value.isNull())
            throw InvalidRequestError("'stream_options' is not supported");

        value = reqValue("temperature", Number, false, /*min*/ 0, /*max*/ 2);
        if (!value.isNull())
            this->temperature = float(value.toDouble());

        value = reqValue("top_p", Number, false, /*min*/ 0, /*max*/ 1);
        if (!value.isNull())
            this->top_p = float(value.toDouble());

        value = reqValue("min_p", Number, false, /*min*/ 0, /*max*/ 1);
        if (!value.isNull())
            this->min_p = float(value.toDouble());

        reqValue("user", String); // validate but don't use
    }

    enum class Type : uint8_t {
        Boolean,
        Integer,
        Number,
        String,
        Array,
        Object,
    };

    static const std::unordered_map<Type, const char *> s_typeNames;

    static bool typeMatches(const QCborValue &value, Type type) noexcept {
        using enum Type;
        switch (type) {
            case Boolean: return value.isBool();
            case Integer: return value.isInteger();
            case Number:  return value.isInteger() || value.isDouble();
            case String:  return value.isString();
            case Array:   return value.isArray();
            case Object:  return value.isMap();
        }
        Q_UNREACHABLE();
    }

    static QCborValue takeValue(
        QCborMap &obj, const char *key, std::optional<Type> type = {}, bool required = false,
        std::optional<qint64> min = {}, std::optional<qint64> max = {}
    ) {
        auto value = obj.take(QLatin1StringView(key));
        if (value.isUndefined())
            value = QCborValue(QCborSimpleType::Null);
        if (required && value.isNull())
            throw InvalidRequestError(fmt::format("you must provide a {} parameter", key));
        if (type && !value.isNull() && !typeMatches(value, *type))
            throw InvalidRequestError(fmt::format("'{}' is not of type '{}' - '{}'",
                                                  value.toVariant(), s_typeNames.at(*type), key));
        if (!value.isNull()) {
            double num = value.toDouble();
            if (min && num < double(*min))
                throw InvalidRequestError(fmt::format("{} is less than the minimum of {} - '{}'", num, *min, key));
            if (max && num > double(*max))
                throw InvalidRequestError(fmt::format("{} is greater than the maximum of {} - '{}'", num, *max, key));
        }
        return value;
    }

private:
    Q_DISABLE_COPY_MOVE(BaseCompletionRequest)
};

class CompletionRequest : public BaseCompletionRequest {
public:
    QString prompt; // required
    // some parameters are not supported yet - these ones are
    bool echo = false;

    CompletionRequest &parse(QCborMap request) override
    {
        BaseCompletionRequest::parse(std::move(request));
        return *this;
    }

protected:
    void parseImpl(QCborMap &request) override
    {
        using enum Type;

        auto reqValue = [&request](auto &&...args) { return takeValue(request, args...); };
        QCborValue value;

        BaseCompletionRequest::parseImpl(request);

        this->prompt = reqValue("prompt", String, /*required*/ true).toString();

        value = reqValue("best_of", Integer);
        {
            qint64 bof = value.toInteger(1);
            if (this->n > bof)
                throw InvalidRequestError(fmt::format(
                    "You requested that the server return more choices than it will generate (HINT: you must set 'n' "
                    "(currently {}) to be at most 'best_of' (currently {}), or omit either parameter if you don't "
                    "specifically want to use them.)",
                    this->n, bof
                ));
            if (bof > this->n)
                throw InvalidRequestError("'best_of' is not supported");
        }

        value = reqValue("echo", Boolean);
        if (value.isBool())
            this->echo = value.toBool();

        // we don't bother deeply typechecking unsupported subobjects for now
        value = reqValue("logit_bias", Object);
        if (!value.isNull())
            throw InvalidRequestError("'logit_bias' is not supported");

        value = reqValue("logprobs", Integer, false, /*min*/ 0);
        if (!value.isNull())
            throw InvalidRequestError("'logprobs' is not supported");

        value = reqValue("suffix", String);
        if (!value.isNull() && !value.toString().isEmpty())
            throw InvalidRequestError("'suffix' is not supported");
    }
};

const std::unordered_map<BaseCompletionRequest::Type, const char *> BaseCompletionRequest::s_typeNames = {
    { BaseCompletionRequest::Type::Boolean, "boolean" },
    { BaseCompletionRequest::Type::Integer, "integer" },
    { BaseCompletionRequest::Type::Number,  "number"  },
    { BaseCompletionRequest::Type::String,  "string"  },
    { BaseCompletionRequest::Type::Array,   "array"   },
    { BaseCompletionRequest::Type::Object,  "object"  },
};

class ChatRequest : public BaseCompletionRequest {
public:
    struct Message {
        enum class Role { System, User, Assistant };
        Role    role;
        QString content;
    };

    QList<Message> messages; // required

    ChatRequest &parse(QCborMap request) override
    {
        BaseCompletionRequest::parse(std::move(request));
        return *this;
    }

protected:
    void parseImpl(QCborMap &request) override
    {
        using enum Type;

        auto reqValue = [&request](auto &&...args) { return takeValue(request, args...); };
        QCborValue value;

        BaseCompletionRequest::parseImpl(request);

        value = reqValue("messages", std::nullopt, /*required*/ true);
        if (!value.isArray() || value.toArray().isEmpty())
            throw InvalidRequestError(fmt::format(
                "Invalid type for 'messages': expected a non-empty array of objects, but got '{}' instead.",
                value.toVariant()
            ));

        this->messages.clear();
        {
            QCborArray arr = value.toArray();
            for (qsizetype i = 0; i < arr.size(); i++) {
                const auto &elem = arr[i];
                if (!elem.isMap())
                    throw InvalidRequestError(fmt::format(
                        "Invalid type for 'messages[{}]': expected an object, but got '{}' instead.",
                        i, elem.toVariant()
                    ));
                QCborMap msg = elem.toMap();
                Message res;
                QString role = takeValue(msg, "role", String, /*required*/ true).toString();
                if (role == u"system"_s) {
                    res.role = Message::Role::System;
                } else if (role == u"user"_s) {
                    res.role = Message::Role::User;
                } else if (role == u"assistant"_s) {
                    res.role = Message::Role::Assistant;
                } else {
                    throw InvalidRequestError(fmt::format(
                        "Invalid 'messages[{}].role': expected one of 'system', 'assistant', or 'user', but got '{}'"
                        " instead.",
                        i, role.toStdString()
                    ));
                }
                res.content = takeValue(msg, "content", String, /*required*/ true).toString();
                this->messages.append(res);

                if (!msg.isEmpty())
                    throw InvalidRequestError(fmt::format(
                        "Invalid 'messages[{}]': unrecognized key: '{}'", i, msg.keys().constFirst().toString()
                    ));
            }
        }

        // we don't bother deeply typechecking unsupported subobjects for now
        value = reqValue("logit_bias", Object);
        if (!value.isNull())
            throw InvalidRequestError("'logit_bias' is not supported");

        value = reqValue("logprobs", Boolean);
        if (value.isTrue())
            throw InvalidRequestError("'logprobs' is not supported");

        value = reqValue("top_logprobs", Integer, false, /*min*/ 0);
        if (!value.isNull())
            throw InvalidRequestError("The 'top_logprobs' parameter is only allowed when 'logprobs' is enabled.");

        value = reqValue("response_format", Object);
        if (!value.isNull())
            throw InvalidRequestError("'response_format' is not supported");

        reqValue("service_tier", String); // validate but don't use

        value = reqValue("tools", Array);
        if (!value.isNull())
            throw InvalidRequestError("'tools' is not supported");

        value = reqValue("tool_choice");
        if (!value.isNull())
            throw InvalidRequestError("'tool_choice' is not supported");

        // validate but don't use
        reqValue("parallel_tool_calls", Boolean);

        value = reqValue("function_call");
        if (!value.isNull())
            throw InvalidRequestError("'function_call' is not supported");

        value = reqValue("functions", Array);
        if (!value.isNull())
            throw InvalidRequestError("'functions' is not supported");
    }
};

template <typename T>
T &parseRequest(T &request, QJsonObject &&obj)
{
    // lossless conversion to CBOR exposes more type information
    return request.parse(QCborMap::fromJsonObject(obj));
}

Server::Server(Chat *chat)
    : ChatLLM(chat, true /*isServer*/)
    , m_chat(chat)
{
    connect(this, &Server::threadStarted, this, &Server::start);
    connect(this, &Server::databaseResultsChanged, this, &Server::handleDatabaseResultsChanged);
    connect(chat, &Chat::collectionListChanged, this, &Server::handleCollectionListChanged, Qt::QueuedConnection);
}

static QJsonObject requestFromJson(const QByteArray &request)
{
    QJsonParseError err;
    const QJsonDocument document = QJsonDocument::fromJson(request, &err);
    if (err.error || !document.isObject())
        throw InvalidRequestError(fmt::format(
            "error parsing request JSON: {}",
            err.error ? err.errorString().toStdString() : "not an object"s
        ));
    return document.object();
}

void Server::start()
{
    m_server = std::make_unique<QHttpServer>(this);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    auto *tcpServer = new QTcpServer(m_server.get());
#else
    auto *tcpServer = m_server.get();
#endif

    auto port = MySettings::globalInstance()->networkPort();
    if (!tcpServer->listen(QHostAddress::LocalHost, port)) {
        qWarning() << "Server ERROR: Failed to listen on port" << port;
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    if (!m_server->bind(tcpServer)) {
        qWarning() << "Server ERROR: Failed to HTTP server to socket" << port;
        return;
    }
#endif

    m_server->route("/v1/models", QHttpServerRequest::Method::Get,
        [](const QHttpServerRequest &) {
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
        [](const QString &model, const QHttpServerRequest &) {
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

            try {
                auto reqObj = requestFromJson(request.body());
#if defined(DEBUG)
                qDebug().noquote() << "/v1/completions request" << QJsonDocument(reqObj).toJson(QJsonDocument::Indented);
#endif
                CompletionRequest req;
                parseRequest(req, std::move(reqObj));
                auto [resp, respObj] = handleCompletionRequest(req);
#if defined(DEBUG)
                if (respObj)
                    qDebug().noquote() << "/v1/completions reply" << QJsonDocument(*respObj).toJson(QJsonDocument::Indented);
#endif
                return std::move(resp);
            } catch (const InvalidRequestError &e) {
                return e.asResponse();
            }
        }
    );

    m_server->route("/v1/chat/completions", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request) {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);

            try {
                auto reqObj = requestFromJson(request.body());
#if defined(DEBUG)
                qDebug().noquote() << "/v1/chat/completions request" << QJsonDocument(reqObj).toJson(QJsonDocument::Indented);
#endif
                ChatRequest req;
                parseRequest(req, std::move(reqObj));
                auto [resp, respObj] = handleChatRequest(req);
                (void)respObj;
#if defined(DEBUG)
                if (respObj)
                    qDebug().noquote() << "/v1/chat/completions reply" << QJsonDocument(*respObj).toJson(QJsonDocument::Indented);
#endif
                return std::move(resp);
            } catch (const InvalidRequestError &e) {
                return e.asResponse();
            }
        }
    );

    // Respond with code 405 to wrong HTTP methods:
    m_server->route("/v1/models",  QHttpServerRequest::Method::Post,
        [] {
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
        [](const QString &model) {
            (void)model;
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
        [] {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
            return QHttpServerResponse(
                QJsonDocument::fromJson("{\"error\": {\"message\": \"Only POST requests are accepted.\","
                    " \"type\": \"invalid_request_error\", \"param\": null, \"code\": \"method_not_supported\"}}").object(),
                QHttpServerResponder::StatusCode::MethodNotAllowed);
        }
    );

    m_server->route("/v1/chat/completions", QHttpServerRequest::Method::Get,
        [] {
            if (!MySettings::globalInstance()->serverChat())
                return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
            return QHttpServerResponse(
                QJsonDocument::fromJson("{\"error\": {\"message\": \"Only POST requests are accepted.\","
                    " \"type\": \"invalid_request_error\", \"param\": null, \"code\": \"method_not_supported\"}}").object(),
                QHttpServerResponder::StatusCode::MethodNotAllowed);
        }
    );

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    m_server->addAfterRequestHandler(this, [](const QHttpServerRequest &req, QHttpServerResponse &resp) {
        Q_UNUSED(req);
        auto headers = resp.headers();
        headers.append("Access-Control-Allow-Origin"_L1, "*"_L1);
        resp.setHeaders(std::move(headers));
    });
#else
    m_server->afterRequest([](QHttpServerResponse &&resp) {
        resp.addHeader("Access-Control-Allow-Origin", "*");
        return std::move(resp);
    });
#endif

    connect(this, &Server::requestResetResponseState, m_chat, &Chat::resetResponseState, Qt::BlockingQueuedConnection);
}

static auto makeError(auto &&...args) -> std::pair<QHttpServerResponse, std::optional<QJsonObject>>
{
    return {QHttpServerResponse(args...), std::nullopt};
}

auto Server::handleCompletionRequest(const CompletionRequest &request)
    -> std::pair<QHttpServerResponse, std::optional<QJsonObject>>
{
    Q_ASSERT(m_chatModel);

    auto *mySettings = MySettings::globalInstance();

    ModelInfo modelInfo = ModelList::globalInstance()->defaultModelInfo();
    const QList<ModelInfo> modelList = ModelList::globalInstance()->selectableModelList();
    for (const ModelInfo &info : modelList) {
        Q_ASSERT(info.installed);
        if (!info.installed)
            continue;
        if (request.model == info.name() || request.model == info.filename()) {
            modelInfo = info;
            break;
        }
    }

    // load the new model if necessary
    setShouldBeLoaded(true);

    if (modelInfo.filename().isEmpty()) {
        std::cerr << "ERROR: couldn't load default model " << request.model.toStdString() << std::endl;
        return makeError(QHttpServerResponder::StatusCode::InternalServerError);
    }

    emit requestResetResponseState(); // blocks
    qsizetype prevMsgIndex = m_chatModel->count() - 1;
    if (prevMsgIndex >= 0)
        m_chatModel->updateCurrentResponse(prevMsgIndex, false);

    // NB: this resets the context, regardless of whether this model is already loaded
    if (!loadModel(modelInfo)) {
        std::cerr << "ERROR: couldn't load model " << modelInfo.name().toStdString() << std::endl;
        return makeError(QHttpServerResponder::StatusCode::InternalServerError);
    }

    // add prompt/response items to GUI
    m_chatModel->appendPrompt(request.prompt);
    m_chatModel->appendResponse();

    // FIXME(jared): taking parameters from the UI inhibits reproducibility of results
    LLModel::PromptContext promptCtx {
        .n_predict      = request.max_tokens,
        .top_k          = mySettings->modelTopK(modelInfo),
        .top_p          = request.top_p,
        .min_p          = request.min_p,
        .temp           = request.temperature,
        .n_batch        = mySettings->modelPromptBatchSize(modelInfo),
        .repeat_penalty = float(mySettings->modelRepeatPenalty(modelInfo)),
        .repeat_last_n  = mySettings->modelRepeatPenaltyTokens(modelInfo),
    };

    auto promptUtf8 = request.prompt.toUtf8();
    int promptTokens = 0;
    int responseTokens = 0;
    QStringList responses;
    for (int i = 0; i < request.n; ++i) {
        PromptResult result;
        try {
            result = promptInternal(std::string_view(promptUtf8.cbegin(), promptUtf8.cend()),
                                    promptCtx,
                                    /*usedLocalDocs*/ false);
        } catch (const std::exception &e) {
            m_chatModel->setResponseValue(e.what());
            m_chatModel->setError();
            emit responseStopped(0);
            return makeError(QHttpServerResponder::StatusCode::InternalServerError);
        }
        QString resp = QString::fromUtf8(result.response);
        if (request.echo)
            resp = request.prompt + resp;
        responses << resp;
        if (i == 0)
            promptTokens = result.promptTokens;
        responseTokens += result.responseTokens;
    }

    QJsonObject responseObject {
        { "id",      "placeholder"                      },
        { "object",  "text_completion"                  },
        { "created", QDateTime::currentSecsSinceEpoch() },
        { "model",   modelInfo.name()                   },
    };

    QJsonArray choices;
    for (qsizetype i = 0; auto &resp : std::as_const(responses)) {
        choices << QJsonObject {
            { "text",          resp                                                     },
            { "index",         i++                                                      },
            { "logprobs",      QJsonValue::Null                                         },
            { "finish_reason", responseTokens == request.max_tokens ? "length" : "stop" },
        };
    }

    responseObject.insert("choices", choices);
    responseObject.insert("usage", QJsonObject {
        { "prompt_tokens",     promptTokens                  },
        { "completion_tokens", responseTokens                },
        { "total_tokens",      promptTokens + responseTokens },
    });

    return {QHttpServerResponse(responseObject), responseObject};
}

auto Server::handleChatRequest(const ChatRequest &request)
    -> std::pair<QHttpServerResponse, std::optional<QJsonObject>>
{
    auto *mySettings = MySettings::globalInstance();

    ModelInfo modelInfo = ModelList::globalInstance()->defaultModelInfo();
    const QList<ModelInfo> modelList = ModelList::globalInstance()->selectableModelList();
    for (const ModelInfo &info : modelList) {
        Q_ASSERT(info.installed);
        if (!info.installed)
            continue;
        if (request.model == info.name() || request.model == info.filename()) {
            modelInfo = info;
            break;
        }
    }

    // load the new model if necessary
    setShouldBeLoaded(true);

    if (modelInfo.filename().isEmpty()) {
        std::cerr << "ERROR: couldn't load default model " << request.model.toStdString() << std::endl;
        return makeError(QHttpServerResponder::StatusCode::InternalServerError);
    }

    emit requestResetResponseState(); // blocks

    // NB: this resets the context, regardless of whether this model is already loaded
    if (!loadModel(modelInfo)) {
        std::cerr << "ERROR: couldn't load model " << modelInfo.name().toStdString() << std::endl;
        return makeError(QHttpServerResponder::StatusCode::InternalServerError);
    }

    m_chatModel->updateCurrentResponse(m_chatModel->count() - 1, false);

    Q_ASSERT(!request.messages.isEmpty());

    // adds prompt/response items to GUI
    std::vector<MessageInput> messages;
    for (auto &message : request.messages) {
        using enum ChatRequest::Message::Role;
        switch (message.role) {
            case System:    messages.push_back({ MessageInput::Type::System,   message.content }); break;
            case User:      messages.push_back({ MessageInput::Type::Prompt,   message.content }); break;
            case Assistant: messages.push_back({ MessageInput::Type::Response, message.content }); break;
        }
    }
    auto startOffset = m_chatModel->appendResponseWithHistory(messages);

    // FIXME(jared): taking parameters from the UI inhibits reproducibility of results
    LLModel::PromptContext promptCtx {
        .n_predict      = request.max_tokens,
        .top_k          = mySettings->modelTopK(modelInfo),
        .top_p          = request.top_p,
        .min_p          = request.min_p,
        .temp           = request.temperature,
        .n_batch        = mySettings->modelPromptBatchSize(modelInfo),
        .repeat_penalty = float(mySettings->modelRepeatPenalty(modelInfo)),
        .repeat_last_n  = mySettings->modelRepeatPenaltyTokens(modelInfo),
    };

    int promptTokens   = 0;
    int responseTokens = 0;
    QList<QPair<QString, QList<ResultInfo>>> responses;
    for (int i = 0; i < request.n; ++i) {
        ChatPromptResult result;
        try {
            result = promptInternalChat(m_collections, promptCtx, startOffset);
        } catch (const std::exception &e) {
            m_chatModel->setResponseValue(e.what());
            m_chatModel->setError();
            emit responseStopped(0);
            return makeError(QHttpServerResponder::StatusCode::InternalServerError);
        }
        responses.emplace_back(result.response, result.databaseResults);
        if (i == 0)
            promptTokens = result.promptTokens;
        responseTokens += result.responseTokens;
    }

    QJsonObject responseObject {
        { "id",      "placeholder"                      },
        { "object",  "chat.completion"                  },
        { "created", QDateTime::currentSecsSinceEpoch() },
        { "model",   modelInfo.name()                   },
    };

    QJsonArray choices;
    {
        int index = 0;
        for (const auto &r : responses) {
            QString result = r.first;
            QList<ResultInfo> infos = r.second;
            QJsonObject message {
                { "role",    "assistant" },
                { "content", result      },
            };
            QJsonObject choice {
                { "index",         index++                                                  },
                { "message",       message                                                  },
                { "finish_reason", responseTokens == request.max_tokens ? "length" : "stop" },
                { "logprobs",      QJsonValue::Null                                         },
            };
            if (MySettings::globalInstance()->localDocsShowReferences()) {
                QJsonArray references;
                for (const auto &ref : infos)
                    references.append(resultToJson(ref));
                choice.insert("references", references.isEmpty() ? QJsonValue::Null : QJsonValue(references));
            }
            choices.append(choice);
        }
    }

    responseObject.insert("choices", choices);
    responseObject.insert("usage", QJsonObject {
        { "prompt_tokens",     promptTokens                  },
        { "completion_tokens", responseTokens                },
        { "total_tokens",      promptTokens + responseTokens },
    });

    return {QHttpServerResponse(responseObject), responseObject};
}
