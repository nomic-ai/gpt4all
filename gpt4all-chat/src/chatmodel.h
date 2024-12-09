#ifndef CHATMODEL_H
#define CHATMODEL_H

#include "database.h"
#include "utils.h"
#include "xlsxtomd.h"

#include <fmt/format.h>

#include <QAbstractListModel>
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVariant>
#include <QVector>
#include <Qt>
#include <QtGlobal>

#include <iterator>
#include <ranges>
#include <span>
#include <utility>

using namespace Qt::Literals::StringLiterals;
namespace ranges = std::ranges;


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

struct ChatItem
{
    Q_GADGET
    Q_PROPERTY(QString name  MEMBER name )
    Q_PROPERTY(QString value MEMBER value)

    // prompts
    Q_PROPERTY(QList<PromptAttachment> promptAttachments MEMBER promptAttachments)
    Q_PROPERTY(QString                 bakedPrompt       READ   bakedPrompt      )

    // responses
    Q_PROPERTY(bool isCurrentResponse MEMBER isCurrentResponse)
    Q_PROPERTY(bool isError           MEMBER isError          )

    // responses (DataLake)
    Q_PROPERTY(QString newResponse     MEMBER newResponse    )
    Q_PROPERTY(bool    stopped         MEMBER stopped        )
    Q_PROPERTY(bool    thumbsUpState   MEMBER thumbsUpState  )
    Q_PROPERTY(bool    thumbsDownState MEMBER thumbsDownState)

public:
    enum class Type { System, Prompt, Response };

    // tags for constructing ChatItems
    struct prompt_tag_t { explicit prompt_tag_t() = default; };
    static inline constexpr prompt_tag_t prompt_tag = prompt_tag_t();
    struct response_tag_t { explicit response_tag_t() = default; };
    static inline constexpr response_tag_t response_tag = response_tag_t();
    struct system_tag_t { explicit system_tag_t() = default; };
    static inline constexpr system_tag_t system_tag = system_tag_t();

    // FIXME(jared): This should not be necessary. QML should see null or undefined if it
    // tries to access something invalid.
    ChatItem() = default;

    // NOTE: system messages are currently never stored in the model or serialized
    ChatItem(system_tag_t, const QString &value)
        : name(u"System: "_s), value(value) {}

    ChatItem(prompt_tag_t, const QString &value, const QList<PromptAttachment> &attachments = {})
        : name(u"Prompt: "_s), value(value), promptAttachments(attachments) {}

    ChatItem(response_tag_t, bool isCurrentResponse = true)
        : name(u"Response: "_s), isCurrentResponse(isCurrentResponse) {}

    Type type() const
    {
        if (name == u"System: "_s)
            return Type::System;
        if (name == u"Prompt: "_s)
            return Type::Prompt;
        if (name == u"Response: "_s)
            return Type::Response;
        throw std::invalid_argument(fmt::format("Chat item has unknown label: {:?}", name));
    }

    // used with version 0 Jinja templates
    QString bakedPrompt() const
    {
        if (type() != Type::Prompt)
            throw std::logic_error("bakedPrompt() called on non-prompt item");
        QStringList parts;
        if (!sources.isEmpty()) {
            parts << u"### Context:\n"_s;
            for (auto &source : std::as_const(sources))
                parts << u"Collection: "_s << source.collection
                      << u"\nPath: "_s     << source.path
                      << u"\nExcerpt: "_s  << source.text << u"\n\n"_s;
        }
        for (auto &attached : std::as_const(promptAttachments))
            parts << attached.processedContent() << u"\n\n"_s;
        parts << value;
        return parts.join(QString());
    }

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

    // responses (DataLake)
    QString newResponse;
    bool    stopped         = false;
    bool    thumbsUpState   = false;
    bool    thumbsDownState = false;
};
Q_DECLARE_METATYPE(ChatItem)

class ChatModelAccessor : public ranges::subrange<QList<ChatItem>::const_iterator> {
private:
    using Super = ranges::subrange<QList<ChatItem>::const_iterator>;

public:
    template <typename... T>
    ChatModelAccessor(QMutex &mutex, T &&...args)
        : Super(std::forward<T>(args)...), m_lock(&mutex) {}

private:
    QMutexLocker<QMutex> m_lock;
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
        PeerRole,

        // prompts
        PromptAttachmentsRole,

        // responses
        // NOTE: sources are stored on the *prompts*, but in the model, they are only on the *responses*!
        SourcesRole,
        ConsolidatedSourcesRole,
        IsCurrentResponseRole,
        IsErrorRole,

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
    auto getPeerUnlocked(QList<ChatItem>::const_iterator item) const
        -> std::optional<QList<ChatItem>::const_iterator>
    {
        switch (item->type()) {
            using enum ChatItem::Type;
        case Prompt:
            {
                auto peer = std::next(item);
                if (peer < m_chatItems.cend() && peer->type() == Response)
                    return peer;
                break;
            }
        case Response:
            {
                if (item > m_chatItems.cbegin()) {
                    if (auto peer = std::prev(item); peer->type() == Prompt)
                        return peer;
                }
                break;
            }
        default:
            throw std::invalid_argument("getPeer() called on item that is not a prompt or response");
        }
        return std::nullopt;
    }

    auto getPeerUnlocked(int index) const -> std::optional<int>
    {
        return getPeerUnlocked(m_chatItems.cbegin() + index)
            .transform([&](auto &&i) { return i - m_chatItems.cbegin(); } );
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        QMutexLocker locker(&m_mutex);
        if (!index.isValid() || index.row() < 0 || index.row() >= m_chatItems.size())
            return QVariant();

        auto item = m_chatItems.cbegin() + index.row();
        switch (role) {
            case NameRole:
                return item->name;
            case ValueRole:
                return item->value;
            case PeerRole:
                switch (item->type()) {
                    using enum ChatItem::Type;
                case Prompt:
                case Response:
                    {
                        auto peer = getPeerUnlocked(item);
                        return peer ? QVariant::fromValue(**peer) : QVariant::fromValue(nullptr);
                    }
                default:
                    return QVariant();
                }
            case PromptAttachmentsRole:
                return QVariant::fromValue(item->promptAttachments);
            case SourcesRole:
                {
                    QList<ResultInfo> data;
                    if (item->type() == ChatItem::Type::Response) {
                        if (auto prompt = getPeerUnlocked(item))
                            data = (*prompt)->consolidatedSources;
                    }
                    return QVariant::fromValue(data);
                }
            case ConsolidatedSourcesRole:
                {
                    QList<ResultInfo> data;
                    if (item->type() == ChatItem::Type::Response) {
                        if (auto prompt = getPeerUnlocked(item))
                            data = (*prompt)->sources;
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
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            { NameRole,                "name"                },
            { ValueRole,               "value"               },
            { PeerRole,                "peer"                },
            { PromptAttachmentsRole,   "promptAttachments"   },
            { SourcesRole,             "sources"             },
            { ConsolidatedSourcesRole, "consolidatedSources" },
            { IsCurrentResponseRole,   "isCurrentResponse"   },
            { IsErrorRole,             "isError"             },
            { NewResponseRole,         "newResponse"         },
            { StoppedRole,             "stopped"             },
            { ThumbsUpStateRole,       "thumbsUpState"       },
            { ThumbsDownStateRole,     "thumbsDownState"     },
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
            m_chatItems.emplace_back(ChatItem::prompt_tag, value, attachments);
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

        int promptIndex = 0;
        beginInsertRows(QModelIndex(), count, count);
        {
            QMutexLocker locker(&m_mutex);
            m_chatItems.emplace_back(ChatItem::response_tag);
            if (auto pi = getPeerUnlocked(m_chatItems.size() - 1))
                promptIndex = *pi;
        }
        endInsertRows();
        emit countChanged();
        if (promptIndex >= 0)
            emit dataChanged(createIndex(promptIndex, 0), createIndex(promptIndex, 0), {PeerRole});
    }

    // Used by Server to append a new conversation to the chat log.
    void appendResponseWithHistory(std::span<const ChatItem> history)
    {
        if (history.empty())
            throw std::invalid_argument("at least one message is required");

        m_mutex.lock();
        qsizetype startIndex = m_chatItems.count();
        m_mutex.unlock();

        qsizetype nNewItems = history.size() + 1;
        qsizetype endIndex  = startIndex + nNewItems;
        beginInsertRows(QModelIndex(), startIndex, endIndex - 1 /*inclusive*/);
        bool hadError;
        {
            QMutexLocker locker(&m_mutex);
            hadError = hasErrorUnlocked();
            m_chatItems.reserve(m_chatItems.count() + nNewItems);
            for (auto &item : history)
                m_chatItems << item;
            m_chatItems.emplace_back(ChatItem::response_tag);
        }
        endInsertRows();
        emit countChanged();
        // Server can add messages when there is an error because each call is a new conversation
        if (hadError)
            emit hasErrorChanged(false);
    }

    void truncate(qsizetype size)
    {
        qsizetype oldSize;
        {
            QMutexLocker locker(&m_mutex);
            if (size >= (oldSize = m_chatItems.size()))
                return;
            if (size && m_chatItems.at(size - 1).type() != ChatItem::Type::Response)
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

    Q_INVOKABLE ChatItem get(int index)
    {
        QMutexLocker locker(&m_mutex);
        if (index < 0 || index >= m_chatItems.size()) return ChatItem();
        return m_chatItems.at(index);
    }

    Q_INVOKABLE void updateCurrentResponse(int index, bool b)
    {
        bool changed = false;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            ChatItem &item = m_chatItems[index];
            if (item.isCurrentResponse != b) {
                item.isCurrentResponse = b;
                changed = true;
            }
        }

        if (changed) emit dataChanged(createIndex(index, 0), createIndex(index, 0), {IsCurrentResponseRole});
    }

    Q_INVOKABLE void updateStopped(int index, bool b)
    {
        bool changed = false;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            ChatItem &item = m_chatItems[index];
            if (item.stopped != b) {
                item.stopped = b;
                changed = true;
            }
        }
        if (changed) emit dataChanged(createIndex(index, 0), createIndex(index, 0), {StoppedRole});
    }

    Q_INVOKABLE void updateValue(int index, const QString &value)
    {
        bool changed = false;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            ChatItem &item = m_chatItems[index];
            if (item.value != value) {
                item.value = value;
                changed = true;
            }
        }
        if (changed) {
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ValueRole});
            emit valueChanged(index, value);
        }
    }

    static QList<ResultInfo> consolidateSources(const QList<ResultInfo> &sources) {
        QMap<QString, ResultInfo> groupedData;
        for (const ResultInfo &info : sources) {
            if (groupedData.contains(info.file)) {
                groupedData[info.file].text += "\n---\n" + info.text;
            } else {
                groupedData[info.file] = info;
            }
        }
        QList<ResultInfo> consolidatedSources = groupedData.values();
        return consolidatedSources;
    }

    Q_INVOKABLE void updateSources(int index, const QList<ResultInfo> &sources)
    {
        int responseIndex = -1;
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            auto promptItem = m_chatItems.begin() + index;
            if (promptItem->type() != ChatItem::Type::Prompt)
                throw std::invalid_argument(fmt::format("item at index {} is not a prompt", index));
            if (auto peer = getPeerUnlocked(promptItem))
                responseIndex = *peer - m_chatItems.cbegin();
            promptItem->sources = sources;
            promptItem->consolidatedSources = consolidateSources(sources);
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

            ChatItem &item = m_chatItems[index];
            if (item.thumbsUpState != b) {
                item.thumbsUpState = b;
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

            ChatItem &item = m_chatItems[index];
            if (item.thumbsDownState != b) {
                item.thumbsDownState = b;
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

            ChatItem &item = m_chatItems[index];
            if (item.newResponse != newResponse) {
                item.newResponse = newResponse;
                changed = true;
            }
        }
        if (changed) emit dataChanged(createIndex(index, 0), createIndex(index, 0), {NewResponseRole});
    }

    Q_INVOKABLE void setError(bool value = true)
    {
        qsizetype index;
        {
            QMutexLocker locker(&m_mutex);

            if (m_chatItems.isEmpty() || m_chatItems.cend()[-1].type() != ChatItem::Type::Response)
                throw std::logic_error("can only set error on a chat that ends with a response");

            index = m_chatItems.count() - 1;
            auto &last = m_chatItems.back();
            if (last.isError == value)
                return; // already set
            last.isError = value;
        }
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {IsErrorRole});
        emit hasErrorChanged(value);
    }

    qsizetype count() const { QMutexLocker locker(&m_mutex); return m_chatItems.size(); }

    ChatModelAccessor chatItems() const { return {m_mutex, std::as_const(m_chatItems)}; }

    bool hasError() const { QMutexLocker locker(&m_mutex); return hasErrorUnlocked(); }

    bool serialize(QDataStream &stream, int version) const
    {
        QMutexLocker locker(&m_mutex);
        stream << int(m_chatItems.size());
        for (auto itemIt = m_chatItems.cbegin(); itemIt < m_chatItems.cend(); ++itemIt) {
            auto c = *itemIt; // NB: copies
            if (version < 11) {
                // move sources from their prompt to the next response
                switch (c.type()) {
                    using enum ChatItem::Type;
                case Prompt:
                    c.sources.clear();
                    c.consolidatedSources.clear();
                    break;
                case Response:
                    // note: we drop sources for responseless prompts
                    if (auto peer = getPeerUnlocked(itemIt)) {
                        c.sources             = (*peer)->sources;
                        c.consolidatedSources = (*peer)->consolidatedSources;
                    }
                default:
                    ;
                }
            }

            // FIXME: This 'id' should be eliminated the next time we bump serialization version.
            // (Jared) This was apparently never used.
            int id = 0;
            stream << id;
            stream << c.name;
            stream << c.value;
            stream << c.newResponse;
            stream << c.isCurrentResponse;
            stream << c.stopped;
            stream << c.thumbsUpState;
            stream << c.thumbsDownState;
            if (version >= 11 && c.type() == ChatItem::Type::Response)
                stream << c.isError;
            if (version >= 8) {
                stream << c.sources.size();
                for (const ResultInfo &info : c.sources) {
                    Q_ASSERT(!info.file.isEmpty());
                    stream << info.collection;
                    stream << info.path;
                    stream << info.file;
                    stream << info.title;
                    stream << info.author;
                    stream << info.date;
                    stream << info.text;
                    stream << info.page;
                    stream << info.from;
                    stream << info.to;
                }
            } else if (version >= 3) {
                QList<QString> references;
                QList<QString> referencesContext;
                int validReferenceNumber = 1;
                for (const ResultInfo &info : c.sources) {
                    if (info.file.isEmpty())
                        continue;

                    QString reference;
                    {
                        QTextStream stream(&reference);
                        stream << (validReferenceNumber++) << ". ";
                        if (!info.title.isEmpty())
                            stream << "\"" << info.title << "\". ";
                        if (!info.author.isEmpty())
                            stream << "By " << info.author << ". ";
                        if (!info.date.isEmpty())
                            stream << "Date: " << info.date << ". ";
                        stream << "In " << info.file << ". ";
                        if (info.page != -1)
                            stream << "Page " << info.page << ". ";
                        if (info.from != -1) {
                            stream << "Lines " << info.from;
                            if (info.to != -1)
                                stream << "-" << info.to;
                            stream << ". ";
                        }
                        stream << "[Context](context://" << validReferenceNumber - 1 << ")";
                    }
                    references.append(reference);
                    referencesContext.append(info.text);
                }

                stream << references.join("\n");
                stream << referencesContext;
            }
            if (version >= 10) {
                stream << c.promptAttachments.size();
                for (const PromptAttachment &a : c.promptAttachments) {
                    Q_ASSERT(!a.url.isEmpty());
                    stream << a.url;
                    stream << a.content;
                }
            }
        }
        return stream.status() == QDataStream::Ok;
    }

    bool deserialize(QDataStream &stream, int version)
    {
        clear(); // reset to known state

        int size;
        stream >> size;
        int lastPromptIndex = -1;
        QList<ChatItem> chatItems;
        for (int i = 0; i < size; ++i) {
            ChatItem c;
            // FIXME: see comment in serialization about id
            int id;
            stream >> id;
            stream >> c.name;
            try {
                c.type(); // check name
            } catch (const std::exception &e) {
                qWarning() << "ChatModel ERROR:" << e.what();
                return false;
            }
            stream >> c.value;
            if (version < 10) {
                // This is deprecated and no longer used
                QString prompt;
                stream >> prompt;
            }
            stream >> c.newResponse;
            stream >> c.isCurrentResponse;
            stream >> c.stopped;
            stream >> c.thumbsUpState;
            stream >> c.thumbsDownState;
            if (version >= 11 && c.type() == ChatItem::Type::Response)
                stream >> c.isError;
            if (version >= 8) {
                qsizetype count;
                stream >> count;
                QList<ResultInfo> sources;
                for (int i = 0; i < count; ++i) {
                    ResultInfo info;
                    stream >> info.collection;
                    stream >> info.path;
                    stream >> info.file;
                    stream >> info.title;
                    stream >> info.author;
                    stream >> info.date;
                    stream >> info.text;
                    stream >> info.page;
                    stream >> info.from;
                    stream >> info.to;
                    sources.append(info);
                }
                c.sources = sources;
                c.consolidatedSources = consolidateSources(sources);
            } else if (version >= 3) {
                QString references;
                QList<QString> referencesContext;
                stream >> references;
                stream >> referencesContext;

                if (!references.isEmpty()) {
                    QList<ResultInfo> sources;
                    QList<QString> referenceList = references.split("\n");

                    // Ignore empty lines and those that begin with "---" which is no longer used
                    for (auto it = referenceList.begin(); it != referenceList.end();) {
                        if (it->trimmed().isEmpty() || it->trimmed().startsWith("---"))
                            it = referenceList.erase(it);
                        else
                            ++it;
                    }

                    Q_ASSERT(referenceList.size() == referencesContext.size());
                    for (int j = 0; j < referenceList.size(); ++j) {
                        QString reference = referenceList[j];
                        QString context = referencesContext[j];
                        ResultInfo info;
                        QTextStream refStream(&reference);
                        QString dummy;
                        int validReferenceNumber;
                        refStream >> validReferenceNumber >> dummy;
                        // Extract title (between quotes)
                        if (reference.contains("\"")) {
                            int startIndex = reference.indexOf('"') + 1;
                            int endIndex = reference.indexOf('"', startIndex);
                            info.title = reference.mid(startIndex, endIndex - startIndex);
                        }

                        // Extract author (after "By " and before the next period)
                        if (reference.contains("By ")) {
                            int startIndex = reference.indexOf("By ") + 3;
                            int endIndex = reference.indexOf('.', startIndex);
                            info.author = reference.mid(startIndex, endIndex - startIndex).trimmed();
                        }

                        // Extract date (after "Date: " and before the next period)
                        if (reference.contains("Date: ")) {
                            int startIndex = reference.indexOf("Date: ") + 6;
                            int endIndex = reference.indexOf('.', startIndex);
                            info.date = reference.mid(startIndex, endIndex - startIndex).trimmed();
                        }

                        // Extract file name (after "In " and before the "[Context]")
                        if (reference.contains("In ") && reference.contains(". [Context]")) {
                            int startIndex = reference.indexOf("In ") + 3;
                            int endIndex = reference.indexOf(". [Context]", startIndex);
                            info.file = reference.mid(startIndex, endIndex - startIndex).trimmed();
                        }

                        // Extract page number (after "Page " and before the next space)
                        if (reference.contains("Page ")) {
                            int startIndex = reference.indexOf("Page ") + 5;
                            int endIndex = reference.indexOf(' ', startIndex);
                            if (endIndex == -1) endIndex = reference.length();
                            info.page = reference.mid(startIndex, endIndex - startIndex).toInt();
                        }

                        // Extract lines (after "Lines " and before the next space or hyphen)
                        if (reference.contains("Lines ")) {
                            int startIndex = reference.indexOf("Lines ") + 6;
                            int endIndex = reference.indexOf(' ', startIndex);
                            if (endIndex == -1) endIndex = reference.length();
                            int hyphenIndex = reference.indexOf('-', startIndex);
                            if (hyphenIndex != -1 && hyphenIndex < endIndex) {
                                info.from = reference.mid(startIndex, hyphenIndex - startIndex).toInt();
                                info.to = reference.mid(hyphenIndex + 1, endIndex - hyphenIndex - 1).toInt();
                            } else {
                                info.from = reference.mid(startIndex, endIndex - startIndex).toInt();
                            }
                        }
                        info.text = context;
                        sources.append(info);
                    }

                    c.sources = sources;
                    c.consolidatedSources = consolidateSources(sources);
                }
            }
            if (version >= 10) {
                qsizetype count;
                stream >> count;
                QList<PromptAttachment> attachments;
                for (int i = 0; i < count; ++i) {
                    PromptAttachment a;
                    stream >> a.url;
                    stream >> a.content;
                    attachments.append(a);
                }
                c.promptAttachments = attachments;
            }

            if (version < 11 && c.type() == ChatItem::Type::Response) {
                // move sources from the response to their last prompt
                if (lastPromptIndex >= 0) {
                    auto &prompt = chatItems[lastPromptIndex];
                    prompt.sources             = std::move(c.sources            );
                    prompt.consolidatedSources = std::move(c.consolidatedSources);
                    lastPromptIndex = -1;
                } else {
                    // drop sources for promptless responses
                    c.sources.clear();
                    c.consolidatedSources.clear();
                }
            }

            chatItems << c;
            if (c.type() == ChatItem::Type::Prompt)
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
    void valueChanged(int index, const QString &value);
    void hasErrorChanged(bool value);

private:
    bool hasErrorUnlocked() const
    {
        if (m_chatItems.isEmpty())
            return false;
        auto &last = m_chatItems.back();
        return last.type() == ChatItem::Type::Response && last.isError;
    }

private:
    mutable QMutex m_mutex;
    QList<ChatItem> m_chatItems;
};

#endif // CHATMODEL_H
