#ifndef CHATMODEL_H
#define CHATMODEL_H

#include "database.h"
#include "tool.h"
#include "toolcallparser.h"
#include "utils.h"
#include "xlsxtomd.h"

#include <fmt/format.h>

#include <QApplication>
#include <QAbstractListModel>
#include <QBuffer>
#include <QByteArray>
#include <QClipboard>
#include <QDataStream>
#include <QJsonDocument>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVariant>
#include <QVector>
#include <Qt>
#include <QtGlobal>

#include <algorithm>
#include <iterator>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

using namespace Qt::Literals::StringLiterals;
namespace ranges = std::ranges;
namespace views  = std::views;


struct PromptAttachment {
    Q_GADGET
    Q_PROPERTY(QUrl url MEMBER url)
    Q_PROPERTY(QByteArray content MEMBER content)
    Q_PROPERTY(QString file READ file)
    Q_PROPERTY(QString processedContent READ processedContent)

public:
    QUrl url;
    QByteArray content;

    QString file() const
    {
        if (!url.isLocalFile())
            return QString();
        const QString localFilePath = url.toLocalFile();
        const QFileInfo info(localFilePath);
        return info.fileName();
    }

    QString processedContent() const
    {
        const QString localFilePath = url.toLocalFile();
        const QFileInfo info(localFilePath);
        if (info.suffix().toLower() != "xlsx")
            return u"## Attached: %1\n\n%2"_s.arg(file(), content);

        QBuffer buffer;
        buffer.setData(content);
        buffer.open(QIODevice::ReadOnly);
        const QString md = XLSXToMD::toMarkdown(&buffer);
        buffer.close();
        return u"## Attached: %1\n\n%2"_s.arg(file(), md);
    }

    bool operator==(const PromptAttachment &other) const { return url == other.url; }
};
Q_DECLARE_METATYPE(PromptAttachment)

// Used by Server to represent a message from the client.
struct MessageInput
{
    enum class Type { System, Prompt, Response };
    Type    type;
    QString content;
};

class MessageItem
{
    Q_GADGET
    Q_PROPERTY(Type    type    READ type    CONSTANT)
    Q_PROPERTY(QString content READ content CONSTANT)

public:
    enum class Type { System, Prompt, Response, ToolResponse };

    MessageItem(Type type, QString content)
        : m_type(type), m_content(std::move(content)) {}

    MessageItem(Type type, QString content, const QList<ResultInfo> &sources, const QList<PromptAttachment> &promptAttachments)
        : m_type(type), m_content(std::move(content)), m_sources(sources), m_promptAttachments(promptAttachments) {}

    Type           type()    const { return m_type;    }
    const QString &content() const { return m_content; }

    QList<ResultInfo>       sources()           const { return m_sources;           }
    QList<PromptAttachment> promptAttachments() const { return m_promptAttachments; }

    // used with version 0 Jinja templates
    QString bakedPrompt() const
    {
        if (type() != Type::Prompt)
            throw std::logic_error("bakedPrompt() called on non-prompt item");
        QStringList parts;
        if (!m_sources.isEmpty()) {
            parts << u"### Context:\n"_s;
            for (auto &source : std::as_const(m_sources))
                parts << u"Collection: "_s << source.collection
                      << u"\nPath: "_s     << source.path
                      << u"\nExcerpt: "_s  << source.text << u"\n\n"_s;
        }
        for (auto &attached : std::as_const(m_promptAttachments))
            parts << attached.processedContent() << u"\n\n"_s;
        parts << m_content;
        return parts.join(QString());
    }

private:
    Type                    m_type;
    QString                 m_content;
    QList<ResultInfo>       m_sources;
    QList<PromptAttachment> m_promptAttachments;
};
Q_DECLARE_METATYPE(MessageItem)

class ChatItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name  MEMBER name )
    Q_PROPERTY(QString value MEMBER value)

    // prompts and responses
    Q_PROPERTY(QString content   READ content NOTIFY contentChanged)

    // prompts
    Q_PROPERTY(QList<PromptAttachment> promptAttachments MEMBER promptAttachments)

    // responses
    Q_PROPERTY(bool                isCurrentResponse   MEMBER isCurrentResponse NOTIFY isCurrentResponseChanged)
    Q_PROPERTY(bool                isError             MEMBER isError          )
    Q_PROPERTY(QList<ChatItem *>   childItems          READ   childItems       )

    // toolcall
    Q_PROPERTY(bool                isToolCallError     READ isToolCallError     NOTIFY isTooCallErrorChanged)

    // responses (DataLake)
    Q_PROPERTY(QString newResponse     MEMBER newResponse    )
    Q_PROPERTY(bool    stopped         MEMBER stopped        )
    Q_PROPERTY(bool    thumbsUpState   MEMBER thumbsUpState  )
    Q_PROPERTY(bool    thumbsDownState MEMBER thumbsDownState)

public:
    enum class Type { System, Prompt, Response, Text, ToolCall, ToolResponse };

    // tags for constructing ChatItems
    struct prompt_tag_t        { explicit prompt_tag_t       () = default; };
    struct response_tag_t      { explicit response_tag_t     () = default; };
    struct system_tag_t        { explicit system_tag_t       () = default; };
    struct text_tag_t          { explicit text_tag_t         () = default; };
    struct tool_call_tag_t     { explicit tool_call_tag_t    () = default; };
    struct tool_response_tag_t { explicit tool_response_tag_t() = default; };
    static inline constexpr prompt_tag_t        prompt_tag        = prompt_tag_t        {};
    static inline constexpr response_tag_t      response_tag      = response_tag_t      {};
    static inline constexpr system_tag_t        system_tag        = system_tag_t        {};
    static inline constexpr text_tag_t          text_tag          = text_tag_t          {};
    static inline constexpr tool_call_tag_t     tool_call_tag     = tool_call_tag_t     {};
    static inline constexpr tool_response_tag_t tool_response_tag = tool_response_tag_t {};

public:
    ChatItem(QObject *parent)
        : QObject(nullptr)
    {
        moveToThread(parent->thread());
        setParent(parent);
    }

    // NOTE: System messages are currently never serialized and only *stored* by the local server.
    //       ChatLLM prepends a system MessageItem on-the-fly.
    ChatItem(QObject *parent, system_tag_t, const QString &value)
        : ChatItem(parent)
    { this->name = u"System: "_s; this->value = value; }

    ChatItem(QObject *parent, prompt_tag_t, const QString &value, const QList<PromptAttachment> &attachments = {})
        : ChatItem(parent)
    { this->name = u"Prompt: "_s; this->value = value; this->promptAttachments = attachments; }

private:
    ChatItem(QObject *parent, response_tag_t, bool isCurrentResponse, const QString &value = {})
        : ChatItem(parent)
    { this->name = u"Response: "_s; this->value = value; this->isCurrentResponse = isCurrentResponse; }

public:
    // A new response, to be filled in
    ChatItem(QObject *parent, response_tag_t)
        : ChatItem(parent, response_tag, true) {}

    // An existing response, from Server
    ChatItem(QObject *parent, response_tag_t, const QString &value)
        : ChatItem(parent, response_tag, false, value) {}

    ChatItem(QObject *parent, text_tag_t, const QString &value)
        : ChatItem(parent)
    { this->name = u"Text: "_s; this->value = value; }

    ChatItem(QObject *parent, tool_call_tag_t, const QString &value)
        : ChatItem(parent)
    { this->name = u"ToolCall: "_s; this->value = value; }

    ChatItem(QObject *parent, tool_response_tag_t, const QString &value)
        : ChatItem(parent)
    { this->name = u"ToolResponse: "_s; this->value = value; }

    Type type() const
    {
        if (name == u"System: "_s)
            return Type::System;
        if (name == u"Prompt: "_s)
            return Type::Prompt;
        if (name == u"Response: "_s)
            return Type::Response;
        if (name == u"Text: "_s)
            return Type::Text;
        if (name == u"ToolCall: "_s)
            return Type::ToolCall;
        if (name == u"ToolResponse: "_s)
            return Type::ToolResponse;
        throw std::invalid_argument(fmt::format("Chat item has unknown label: {:?}", name));
    }

    QString flattenedContent() const
    {
        if (subItems.empty())
            return value;

        // We only flatten one level
        QString content;
        for (ChatItem *item : subItems)
            content += item->value;
        return content;
    }

    QString content() const
    {
        if (type() == Type::Response) {
            // We parse if this contains any part of a partial toolcall
            ToolCallParser parser;
            parser.update(value);

            // If no tool call is detected, return the original value
            if (parser.startIndex() < 0)
                return value;

            // Otherwise we only return the text before and any partial tool call
            const QString beforeToolCall = value.left(parser.startIndex());
            return beforeToolCall;
        }

        // For tool calls we only return content if it is the code interpreter
        if (type() == Type::ToolCall)
            return codeInterpreterContent(value);

        // We don't show any of content from the tool response in the GUI
        if (type() == Type::ToolResponse)
            return QString();

        return value;
    }

    QString codeInterpreterContent(const QString &value) const
    {
        ToolCallParser parser;
        parser.update(value);

        // Extract the code
        QString code = parser.toolCall();
        code = code.trimmed();

        QString result;

        // If we've finished the tool call then extract the result from meta information
        if (toolCallInfo.name == ToolCallConstants::CodeInterpreterFunction)
            result = "```\n" + toolCallInfo.result + "```";

        // Return the formatted code and the result if available
        return code + result;
    }

    QString clipboardContent() const
    {
        QStringList clipContent;
        for (const ChatItem *item : subItems)
            clipContent << item->clipboardContent();
        clipContent << content();
        return clipContent.join("");
    }

    QList<ChatItem *> childItems() const
    {
        // We currently have leaf nodes at depth 3 with nodes at depth 2 as mere containers we don't
        // care about in GUI
        QList<ChatItem *> items;
        for (const ChatItem *item : subItems) {
            items.reserve(items.size() + item->subItems.size());
            ranges::copy(item->subItems, std::back_inserter(items));
        }
        return items;
    }

    QString possibleToolCall() const
    {
        if (!subItems.empty())
            return subItems.back()->possibleToolCall();
        if (type() == Type::ToolCall)
            return value;
        else
            return QString();
    }

    void setCurrentResponse(bool b)
    {
        if (!subItems.empty())
            subItems.back()->setCurrentResponse(b);
        isCurrentResponse = b;
        emit isCurrentResponseChanged();
    }

    void setValue(const QString &v)
    {
        if (!subItems.empty() && subItems.back()->isCurrentResponse) {
            subItems.back()->setValue(v);
            return;
        }

        value = v;
        emit contentChanged();
    }

    void setToolCallInfo(const ToolCallInfo &info)
    {
        toolCallInfo = info;
        emit contentChanged();
        emit isTooCallErrorChanged();
    }

    bool isToolCallError() const
    {
        return toolCallInfo.error != ToolEnums::Error::NoError;
    }

    // NB: Assumes response is not current.
    static ChatItem *fromMessageInput(QObject *parent, const MessageInput &message)
    {
        switch (message.type) {
            using enum MessageInput::Type;
            case Prompt:   return new ChatItem(parent, prompt_tag,   message.content);
            case Response: return new ChatItem(parent, response_tag, message.content);
            case System:   return new ChatItem(parent, system_tag,   message.content);
        }
        Q_UNREACHABLE();
    }

    MessageItem asMessageItem() const
    {
        MessageItem::Type msgType;
        switch (auto typ = type()) {
            using enum ChatItem::Type;
            case System:       msgType = MessageItem::Type::System;       break;
            case Prompt:       msgType = MessageItem::Type::Prompt;       break;
            case Response:     msgType = MessageItem::Type::Response;     break;
            case ToolResponse: msgType = MessageItem::Type::ToolResponse; break;
            case Text:
            case ToolCall:
                throw std::invalid_argument(fmt::format("cannot convert ChatItem type {} to message item", int(typ)));
        }
        return { msgType, flattenedContent(), sources, promptAttachments };
    }

    static QList<ResultInfo> consolidateSources(const QList<ResultInfo> &sources);

    void serializeResponse(QDataStream &stream, int version);
    void serializeToolCall(QDataStream &stream, int version);
    void serializeToolResponse(QDataStream &stream, int version);
    void serializeText(QDataStream &stream, int version);
    void serializeSubItems(QDataStream &stream, int version); // recursive
    void serialize(QDataStream &stream, int version);


    bool deserializeResponse(QDataStream &stream, int version);
    bool deserializeToolCall(QDataStream &stream, int version);
    bool deserializeToolResponse(QDataStream &stream, int version);
    bool deserializeText(QDataStream &stream, int version);
    bool deserializeSubItems(QDataStream &stream, int version); // recursive
    bool deserialize(QDataStream &stream, int version);

Q_SIGNALS:
    void contentChanged();
    void isTooCallErrorChanged();
    void isCurrentResponseChanged();

public:

    // TODO: Maybe we should include the model name here as well as timestamp?
    QString name;
    QString value;

    // prompts
    QList<ResultInfo>       sources;
    QList<ResultInfo>       consolidatedSources;
    QList<PromptAttachment> promptAttachments;

    // responses
    bool isCurrentResponse = false;
    bool isError           = false;
    ToolCallInfo toolCallInfo;
    std::list<ChatItem *> subItems;

    // responses (DataLake)
    QString newResponse;
    bool    stopped         = false;
    bool    thumbsUpState   = false;
    bool    thumbsDownState = false;
};

class ChatModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY hasErrorChanged)

public:
    explicit ChatModel(QObject *parent = nullptr)
        : QAbstractListModel(parent) {}

    // FIXME(jared): can't this start at Qt::UserRole (no +1)?
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ValueRole,

        // prompts and responses
        ContentRole,

        // prompts
        PromptAttachmentsRole,

        // responses
        // NOTE: sources are stored on the *prompts*, but in the model, they are only on the *responses*!
        SourcesRole,
        ConsolidatedSourcesRole,
        IsCurrentResponseRole,
        IsErrorRole,
        ChildItemsRole,

        // responses (DataLake)
        NewResponseRole,
        StoppedRole,
        ThumbsUpStateRole,
        ThumbsDownStateRole,
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        QMutexLocker locker(&m_mutex);
        Q_UNUSED(parent)
        return m_chatItems.size();
    }

    /* a "peer" is a bidirectional 1:1 link between a prompt and the response that would cite its LocalDocs
     * sources. Return std::nullopt if there is none, which is possible for e.g. server chats. */
    template <typename T>
    static std::optional<qsizetype> getPeer(const T *arr, qsizetype size, qsizetype index)
    {
        Q_ASSERT(index >= 0);
        Q_ASSERT(index < size);
        return getPeerInternal(arr, size, index);
    }

private:
    static std::optional<qsizetype> getPeerInternal(const ChatItem * const *arr, qsizetype size, qsizetype index)
    {
        qsizetype peer;
        ChatItem::Type expected;
        switch (arr[index]->type()) {
            using enum ChatItem::Type;
            case Prompt:   peer = index + 1; expected = Response; break;
            case Response: peer = index - 1; expected = Prompt;   break;
            default: throw std::invalid_argument("getPeer() called on item that is not a prompt or response");
        }
        if (peer >= 0 && peer < size && arr[peer]->type() == expected)
            return peer;
        return std::nullopt;
    }

    static std::optional<qsizetype> getPeerInternal(const MessageItem *arr, qsizetype size, qsizetype index)
    {
        qsizetype peer;
        MessageItem::Type expected;
        switch (arr[index].type()) {
            using enum MessageItem::Type;
            case Prompt:   peer = index + 1; expected = Response; break;
            case Response: peer = index - 1; expected = Prompt;   break;
            default: throw std::invalid_argument("getPeer() called on item that is not a prompt or response");
        }
        if (peer >= 0 && peer < size && arr[peer].type() == expected)
            return peer;
        return std::nullopt;
    }

public:
    template <ranges::contiguous_range R>
    static auto getPeer(R &&range, ranges::iterator_t<R> item) -> std::optional<ranges::iterator_t<R>>
    {
        auto begin = ranges::begin(range);
        return getPeer(ranges::data(range), ranges::size(range), item - begin)
            .transform([&](auto i) { return begin + i; });
    }

    auto getPeerUnlocked(QList<ChatItem *>::const_iterator item) const -> std::optional<QList<ChatItem *>::const_iterator>
    { return getPeer(m_chatItems, item); }

    std::optional<qsizetype> getPeerUnlocked(qsizetype index) const
    { return getPeer(m_chatItems.constData(), m_chatItems.size(), index); }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        QMutexLocker locker(&m_mutex);
        if (!index.isValid() || index.row() < 0 || index.row() >= m_chatItems.size())
            return QVariant();

        auto itemIt = m_chatItems.cbegin() + index.row();
        auto *item = *itemIt;
        switch (role) {
            case NameRole:
                return item->name;
            case ValueRole:
                return item->value;
            case PromptAttachmentsRole:
                return QVariant::fromValue(item->promptAttachments);
            case SourcesRole:
                {
                    QList<ResultInfo> data;
                    if (item->type() == ChatItem::Type::Response) {
                        if (auto prompt = getPeerUnlocked(itemIt))
                            data = (**prompt)->sources;
                    }
                    return QVariant::fromValue(data);
                }
            case ConsolidatedSourcesRole:
                {
                    QList<ResultInfo> data;
                    if (item->type() == ChatItem::Type::Response) {
                        if (auto prompt = getPeerUnlocked(itemIt))
                            data = (**prompt)->consolidatedSources;
                    }
                    return QVariant::fromValue(data);
                }
            case IsCurrentResponseRole:
                return item->isCurrentResponse;
            case NewResponseRole:
                return item->newResponse;
            case StoppedRole:
                return item->stopped;
            case ThumbsUpStateRole:
                return item->thumbsUpState;
            case ThumbsDownStateRole:
                return item->thumbsDownState;
            case IsErrorRole:
                return item->type() == ChatItem::Type::Response && item->isError;
            case ContentRole:
                return item->content();
            case ChildItemsRole:
                return QVariant::fromValue(item->childItems());
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            { NameRole,                "name"                },
            { ValueRole,               "value"               },
            { PromptAttachmentsRole,   "promptAttachments"   },
            { SourcesRole,             "sources"             },
            { ConsolidatedSourcesRole, "consolidatedSources" },
            { IsCurrentResponseRole,   "isCurrentResponse"   },
            { IsErrorRole,             "isError"             },
            { NewResponseRole,         "newResponse"         },
            { StoppedRole,             "stopped"             },
            { ThumbsUpStateRole,       "thumbsUpState"       },
            { ThumbsDownStateRole,     "thumbsDownState"     },
            { ContentRole,             "content"             },
            { ChildItemsRole,          "childItems"          },
        };
    }

    void appendPrompt(const QString &value, const QList<PromptAttachment> &attachments = {})
    {
        qsizetype count;
        {
            QMutexLocker locker(&m_mutex);
            if (hasErrorUnlocked())
                throw std::logic_error("cannot append to a failed chat");
            count = m_chatItems.count();
        }

        beginInsertRows(QModelIndex(), count, count);
        {
            QMutexLocker locker(&m_mutex);
            m_chatItems << new ChatItem(this, ChatItem::prompt_tag, value, attachments);
        }
        endInsertRows();
        emit countChanged();
    }

    void appendResponse()
    {
        qsizetype count;
        {
            QMutexLocker locker(&m_mutex);
            if (hasErrorUnlocked())
                throw std::logic_error("cannot append to a failed chat");
            count = m_chatItems.count();
        }

        beginInsertRows(QModelIndex(), count, count);
        {
            QMutexLocker locker(&m_mutex);
            m_chatItems << new ChatItem(this, ChatItem::response_tag);
        }
        endInsertRows();
        emit countChanged();
    }

    // Used by Server to append a new conversation to the chat log.
    // Returns the offset of the appended items.
    qsizetype appendResponseWithHistory(std::span<const MessageInput> history)
    {
        if (history.empty())
            throw std::invalid_argument("at least one message is required");

        m_mutex.lock();
        qsizetype startIndex = m_chatItems.size();
        m_mutex.unlock();

        qsizetype nNewItems = history.size() + 1;
        qsizetype endIndex  = startIndex + nNewItems;
        beginInsertRows(QModelIndex(), startIndex, endIndex - 1 /*inclusive*/);
        bool hadError;
        QList<ChatItem> newItems;
        {
            QMutexLocker locker(&m_mutex);
            startIndex = m_chatItems.size(); // just in case
            hadError = hasErrorUnlocked();
            m_chatItems.reserve(m_chatItems.count() + nNewItems);
            for (auto &message : history)
                m_chatItems << ChatItem::fromMessageInput(this, message);
            m_chatItems << new ChatItem(this, ChatItem::response_tag);
        }
        endInsertRows();
        emit countChanged();
        // Server can add messages when there is an error because each call is a new conversation
        if (hadError)
            emit hasErrorChanged(false);
        return startIndex;
    }

    void truncate(qsizetype size)
    {
        qsizetype oldSize;
        {
            QMutexLocker locker(&m_mutex);
            if (size >= (oldSize = m_chatItems.size()))
                return;
            if (size && m_chatItems.at(size - 1)->type() != ChatItem::Type::Response)
                throw std::invalid_argument(
                    fmt::format("chat model truncated to {} items would not end in a response", size)
                );
        }

        bool oldHasError;
        beginRemoveRows(QModelIndex(), size, oldSize - 1 /*inclusive*/);
        {
            QMutexLocker locker(&m_mutex);
            oldHasError = hasErrorUnlocked();
            Q_ASSERT(size < m_chatItems.size());
            m_chatItems.resize(size);
        }
        endRemoveRows();
        emit countChanged();
        if (oldHasError)
            emit hasErrorChanged(false);
    }

    QString popPrompt(int index)
    {
        QString content;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size() || m_chatItems[index]->type() != ChatItem::Type::Prompt)
                throw std::logic_error("attempt to pop a prompt, but this is not a prompt");
            content = m_chatItems[index]->content();
        }
        truncate(index);
        return content;
    }

    bool regenerateResponse(int index)
    {
        int promptIdx;
        {
            QMutexLocker locker(&m_mutex);
            auto items = m_chatItems; // holds lock
            if (index < 1 || index >= items.size() || items[index]->type() != ChatItem::Type::Response)
                return false;
            promptIdx = getPeerUnlocked(index).value_or(-1);
        }

        truncate(index + 1);
        clearSubItems(index);
        setResponseValue({});
        updateCurrentResponse(index, true );
        updateNewResponse    (index, {}   );
        updateStopped        (index, false);
        updateThumbsUpState  (index, false);
        updateThumbsDownState(index, false);
        setError(false);
        if (promptIdx >= 0)
            updateSources(promptIdx, {});
        return true;
    }

    Q_INVOKABLE void clear()
    {
        {
            QMutexLocker locker(&m_mutex);
            if (m_chatItems.isEmpty()) return;
        }

        bool oldHasError;
        beginResetModel();
        {
            QMutexLocker locker(&m_mutex);
            oldHasError = hasErrorUnlocked();
            m_chatItems.clear();
        }
        endResetModel();
        emit countChanged();
        if (oldHasError)
            emit hasErrorChanged(false);
    }

    Q_INVOKABLE QString possibleToolcall() const
    {
        QMutexLocker locker(&m_mutex);
        if (m_chatItems.empty()) return QString();
        return m_chatItems.back()->possibleToolCall();
    }

    Q_INVOKABLE void updateCurrentResponse(int index, bool b)
    {
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            ChatItem *item = m_chatItems[index];
            item->setCurrentResponse(b);
        }

        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {IsCurrentResponseRole});
    }

    Q_INVOKABLE void updateStopped(int index, bool b)
    {
        bool changed = false;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            ChatItem *item = m_chatItems[index];
            if (item->stopped != b) {
                item->stopped = b;
                changed = true;
            }
        }
        if (changed) emit dataChanged(createIndex(index, 0), createIndex(index, 0), {StoppedRole});
    }

    Q_INVOKABLE void setResponseValue(const QString &value)
    {
        qsizetype index;
        {
            QMutexLocker locker(&m_mutex);
            if (m_chatItems.isEmpty() || m_chatItems.cend()[-1]->type() != ChatItem::Type::Response)
                throw std::logic_error("we only set this on a response");

            index = m_chatItems.count() - 1;
            ChatItem *item = m_chatItems.back();
            item->setValue(value);
        }
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ValueRole, ContentRole});
    }

    Q_INVOKABLE void updateSources(int index, const QList<ResultInfo> &sources)
    {
        int responseIndex = -1;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            auto promptItem = m_chatItems.begin() + index;
            if ((*promptItem)->type() != ChatItem::Type::Prompt)
                throw std::invalid_argument(fmt::format("item at index {} is not a prompt", index));
            if (auto peer = getPeerUnlocked(promptItem))
                responseIndex = *peer - m_chatItems.cbegin();
            (*promptItem)->sources = sources;
            (*promptItem)->consolidatedSources = ChatItem::consolidateSources(sources);
        }
        if (responseIndex >= 0) {
            emit dataChanged(createIndex(responseIndex, 0), createIndex(responseIndex, 0), {SourcesRole});
            emit dataChanged(createIndex(responseIndex, 0), createIndex(responseIndex, 0), {ConsolidatedSourcesRole});
        }
    }

    Q_INVOKABLE void updateThumbsUpState(int index, bool b)
    {
        bool changed = false;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            ChatItem *item = m_chatItems[index];
            if (item->thumbsUpState != b) {
                item->thumbsUpState = b;
                changed = true;
            }
        }
        if (changed) emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ThumbsUpStateRole});
    }

    Q_INVOKABLE void updateThumbsDownState(int index, bool b)
    {
        bool changed = false;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            ChatItem *item = m_chatItems[index];
            if (item->thumbsDownState != b) {
                item->thumbsDownState = b;
                changed = true;
            }
        }
        if (changed) emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ThumbsDownStateRole});
    }

    Q_INVOKABLE void updateNewResponse(int index, const QString &newResponse)
    {
        bool changed = false;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            ChatItem *item = m_chatItems[index];
            if (item->newResponse != newResponse) {
                item->newResponse = newResponse;
                changed = true;
            }
        }
        if (changed) emit dataChanged(createIndex(index, 0), createIndex(index, 0), {NewResponseRole});
    }

    Q_INVOKABLE void splitToolCall(const QPair<QString, QString> &split)
    {
        qsizetype index;
        {
            QMutexLocker locker(&m_mutex);
            if (m_chatItems.isEmpty() || m_chatItems.cend()[-1]->type() != ChatItem::Type::Response)
                throw std::logic_error("can only set toolcall on a chat that ends with a response");

            index = m_chatItems.count() - 1;
            ChatItem *currentResponse = m_chatItems.back();
            Q_ASSERT(currentResponse->isCurrentResponse);

            // Create a new response container for any text and the tool call
            ChatItem *newResponse = new ChatItem(this, ChatItem::response_tag);

            // Add preceding text if any
            if (!split.first.isEmpty()) {
                ChatItem *textItem = new ChatItem(this, ChatItem::text_tag, split.first);
                newResponse->subItems.push_back(textItem);
            }

            // Add the toolcall
            Q_ASSERT(!split.second.isEmpty());
            ChatItem *toolCallItem = new ChatItem(this, ChatItem::tool_call_tag, split.second);
            toolCallItem->isCurrentResponse = true;
            newResponse->subItems.push_back(toolCallItem);

            // Add new response and reset our value
            currentResponse->subItems.push_back(newResponse);
            currentResponse->value = QString();
        }

        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ChildItemsRole, ContentRole});
    }

    Q_INVOKABLE void updateToolCall(const ToolCallInfo &toolCallInfo)
    {
        qsizetype index;
        {
            QMutexLocker locker(&m_mutex);
            if (m_chatItems.isEmpty() || m_chatItems.cend()[-1]->type() != ChatItem::Type::Response)
                throw std::logic_error("can only set toolcall on a chat that ends with a response");

            index = m_chatItems.count() - 1;
            ChatItem *currentResponse = m_chatItems.back();
            Q_ASSERT(currentResponse->isCurrentResponse);

            ChatItem *subResponse = currentResponse->subItems.back();
            Q_ASSERT(subResponse->type() == ChatItem::Type::Response);
            Q_ASSERT(subResponse->isCurrentResponse);

            ChatItem *toolCallItem = subResponse->subItems.back();
            Q_ASSERT(toolCallItem->type() == ChatItem::Type::ToolCall);
            toolCallItem->setToolCallInfo(toolCallInfo);
            toolCallItem->setCurrentResponse(false);

            // Add tool response
            ChatItem *toolResponseItem = new ChatItem(this, ChatItem::tool_response_tag, toolCallInfo.result);
            currentResponse->subItems.push_back(toolResponseItem);
        }

        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ChildItemsRole, ContentRole});
    }

    void clearSubItems(int index)
    {
        bool changed = false;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;
            if (m_chatItems.isEmpty() || m_chatItems[index]->type() != ChatItem::Type::Response)
                throw std::logic_error("can only clear subitems on a chat that ends with a response");

            ChatItem *item = m_chatItems.back();
            if (!item->subItems.empty()) {
                item->subItems.clear();
                changed = true;
            }
        }
        if (changed) {
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ChildItemsRole, ContentRole});
        }
    }

    Q_INVOKABLE void setError(bool value = true)
    {
        qsizetype index;
        {
            QMutexLocker locker(&m_mutex);

            if (m_chatItems.isEmpty() || m_chatItems.cend()[-1]->type() != ChatItem::Type::Response)
                throw std::logic_error("can only set error on a chat that ends with a response");

            index = m_chatItems.count() - 1;
            auto &last = m_chatItems.back();
            if (last->isError == value)
                return; // already set
            last->isError = value;
        }
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {IsErrorRole});
        emit hasErrorChanged(value);
    }

    Q_INVOKABLE void copyToClipboard()
    {
        QMutexLocker locker(&m_mutex);
        QString conversation;
        for (ChatItem *item : m_chatItems) {
            QString string = item->name;
            string += item->clipboardContent();
            string += "\n";
            conversation += string;
        }
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(conversation, QClipboard::Clipboard);
    }

    Q_INVOKABLE void copyToClipboard(int index)
    {
        QMutexLocker locker(&m_mutex);
        if (index < 0 || index >= m_chatItems.size())
            return;
        ChatItem *item = m_chatItems.at(index);
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(item->clipboardContent(), QClipboard::Clipboard);
    }

    qsizetype count() const { QMutexLocker locker(&m_mutex); return m_chatItems.size(); }

    std::vector<MessageItem> messageItems() const
    {
        // A flattened version of the chat item tree used by the backend and jinja
        QMutexLocker locker(&m_mutex);
        std::vector<MessageItem> chatItems;
        for (const ChatItem *item : m_chatItems) {
            chatItems.reserve(chatItems.size() + item->subItems.size() + 1);
            ranges::copy(item->subItems | views::transform(&ChatItem::asMessageItem), std::back_inserter(chatItems));
            chatItems.push_back(item->asMessageItem());
        }
        return chatItems;
    }

    bool hasError() const { QMutexLocker locker(&m_mutex); return hasErrorUnlocked(); }

    bool serialize(QDataStream &stream, int version) const
    {
        // FIXME: need to serialize new chatitem tree
        QMutexLocker locker(&m_mutex);
        stream << int(m_chatItems.size());
        for (auto itemIt = m_chatItems.cbegin(); itemIt < m_chatItems.cend(); ++itemIt) {
            auto c = *itemIt; // NB: copies
            if (version < 11) {
                // move sources from their prompt to the next response
                switch (c->type()) {
                    using enum ChatItem::Type;
                case Prompt:
                    c->sources.clear();
                    c->consolidatedSources.clear();
                    break;
                case Response:
                    // note: we drop sources for responseless prompts
                    if (auto peer = getPeerUnlocked(itemIt)) {
                        c->sources             = (**peer)->sources;
                        c->consolidatedSources = (**peer)->consolidatedSources;
                    }
                default:
                    ;
                }
            }

            c->serialize(stream, version);
        }
        return stream.status() == QDataStream::Ok;
    }

    bool deserialize(QDataStream &stream, int version)
    {
        clear(); // reset to known state

        int size;
        stream >> size;
        int lastPromptIndex = -1;
        QList<ChatItem*> chatItems;
        for (int i = 0; i < size; ++i) {
            ChatItem *c = new ChatItem(this);
            if (!c->deserialize(stream, version)) {
                delete c;
                return false;
            }
            if (version < 11 && c->type() == ChatItem::Type::Response) {
                // move sources from the response to their last prompt
                if (lastPromptIndex >= 0) {
                    auto &prompt = chatItems[lastPromptIndex];
                    prompt->sources             = std::move(c->sources            );
                    prompt->consolidatedSources = std::move(c->consolidatedSources);
                    lastPromptIndex = -1;
                } else {
                    // drop sources for promptless responses
                    c->sources.clear();
                    c->consolidatedSources.clear();
                }
            }

            chatItems << c;
            if (c->type() == ChatItem::Type::Prompt)
                lastPromptIndex = chatItems.size() - 1;
        }

        bool hasError;
        beginInsertRows(QModelIndex(), 0, chatItems.size() - 1 /*inclusive*/);
        {
            QMutexLocker locker(&m_mutex);
            m_chatItems = chatItems;
            hasError = hasErrorUnlocked();
        }
        endInsertRows();
        emit countChanged();
        if (hasError)
            emit hasErrorChanged(true);
        return stream.status() == QDataStream::Ok;
    }

Q_SIGNALS:
    void countChanged();
    void hasErrorChanged(bool value);

private:
    bool hasErrorUnlocked() const
    {
        if (m_chatItems.isEmpty())
            return false;
        auto &last = m_chatItems.back();
        return last->type() == ChatItem::Type::Response && last->isError;
    }

private:
    mutable QMutex m_mutex;
    QList<ChatItem *> m_chatItems;
};

#endif // CHATMODEL_H
