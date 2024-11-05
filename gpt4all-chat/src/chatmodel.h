#ifndef CHATMODEL_H
#define CHATMODEL_H

#include "database.h"
#include "xlsxtomd.h"

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
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString value MEMBER value)
    Q_PROPERTY(QString newResponse MEMBER newResponse)
    Q_PROPERTY(bool currentResponse MEMBER currentResponse)
    Q_PROPERTY(bool stopped MEMBER stopped)
    Q_PROPERTY(bool thumbsUpState MEMBER thumbsUpState)
    Q_PROPERTY(bool thumbsDownState MEMBER thumbsDownState)
    Q_PROPERTY(QList<ResultInfo> sources MEMBER sources)
    Q_PROPERTY(QList<ResultInfo> consolidatedSources MEMBER consolidatedSources)
    Q_PROPERTY(QList<PromptAttachment> promptAttachments MEMBER promptAttachments)
    Q_PROPERTY(QString promptPlusAttachments READ promptPlusAttachments)

public:
    QString promptPlusAttachments() const
    {
        QStringList attachedContexts;
        for (auto attached : promptAttachments)
            attachedContexts << attached.processedContent();

        QString promptPlus = value;
        if (!attachedContexts.isEmpty())
            promptPlus = attachedContexts.join("\n\n") + "\n\n" + value;
        return promptPlus;
    }

    // TODO: Maybe we should include the model name here as well as timestamp?
    QString name;
    QString value;
    QString newResponse;
    QList<ResultInfo> sources;
    QList<ResultInfo> consolidatedSources;
    QList<PromptAttachment> promptAttachments;
    bool currentResponse = false;
    bool stopped = false;
    bool thumbsUpState = false;
    bool thumbsDownState = false;
};
Q_DECLARE_METATYPE(ChatItem)

using ChatModelIterator = QList<ChatItem>::const_iterator;

class ChatModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit ChatModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    enum Roles {
        NameRole = Qt::UserRole + 1,
        ValueRole,
        NewResponseRole,
        CurrentResponseRole,
        StoppedRole,
        ThumbsUpStateRole,
        ThumbsDownStateRole,
        SourcesRole,
        ConsolidatedSourcesRole,
        PromptAttachmentsRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        QMutexLocker locker(&m_mutex);
        Q_UNUSED(parent)
        return m_chatItems.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        QMutexLocker locker(&m_mutex);
        if (!index.isValid() || index.row() < 0 || index.row() >= m_chatItems.size())
            return QVariant();

        const ChatItem &item = m_chatItems.at(index.row());
        switch (role) {
            case NameRole:
                return item.name;
            case ValueRole:
                return item.value;
            case NewResponseRole:
                return item.newResponse;
            case CurrentResponseRole:
                return item.currentResponse;
            case StoppedRole:
                return item.stopped;
            case ThumbsUpStateRole:
                return item.thumbsUpState;
            case ThumbsDownStateRole:
                return item.thumbsDownState;
            case SourcesRole:
                return QVariant::fromValue(item.sources);
            case ConsolidatedSourcesRole:
                return QVariant::fromValue(item.consolidatedSources);
            case PromptAttachmentsRole:
                return QVariant::fromValue(item.promptAttachments);
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[NameRole] = "name";
        roles[ValueRole] = "value";
        roles[NewResponseRole] = "newResponse";
        roles[CurrentResponseRole] = "currentResponse";
        roles[StoppedRole] = "stopped";
        roles[ThumbsUpStateRole] = "thumbsUpState";
        roles[ThumbsDownStateRole] = "thumbsDownState";
        roles[SourcesRole] = "sources";
        roles[ConsolidatedSourcesRole] = "consolidatedSources";
        roles[PromptAttachmentsRole] = "promptAttachments";
        return roles;
    }

    void appendPrompt(const QString &name, const QString &value, const QList<PromptAttachment> &attachments)
    {
        ChatItem item;
        item.name = name;
        item.value = value;
        item.promptAttachments << attachments;

        m_mutex.lock();
        const int count = m_chatItems.count();
        m_mutex.unlock();
        beginInsertRows(QModelIndex(), count, count);
        {
            QMutexLocker locker(&m_mutex);
            m_chatItems.append(item);
        }
        endInsertRows();
        emit countChanged();
    }

    void appendResponse(const QString &name)
    {
        m_mutex.lock();
        const int count = m_chatItems.count();
        m_mutex.unlock();
        ChatItem item;
        item.name = name;
        item.currentResponse = true;
        beginInsertRows(QModelIndex(), count, count);
        {
            QMutexLocker locker(&m_mutex);
            m_chatItems.append(item);
        }
        endInsertRows();
        emit countChanged();
    }

    Q_INVOKABLE void clear()
    {
        {
            QMutexLocker locker(&m_mutex);
            if (m_chatItems.isEmpty()) return;
        }

        beginResetModel();
        {
            QMutexLocker locker(&m_mutex);
            m_chatItems.clear();
        }
        endResetModel();
        emit countChanged();
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
            if (item.currentResponse != b) {
                item.currentResponse = b;
                changed = true;
            }
        }

        if (changed) emit dataChanged(createIndex(index, 0), createIndex(index, 0), {CurrentResponseRole});
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
        {
            QMutexLocker locker(&m_mutex);
            if (index < 0 || index >= m_chatItems.size()) return;

            ChatItem &item = m_chatItems[index];
            item.sources = sources;
            item.consolidatedSources = consolidateSources(sources);
        }
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {SourcesRole});
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ConsolidatedSourcesRole});
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

    int count() const { QMutexLocker locker(&m_mutex); return m_chatItems.size(); }

    ChatModelIterator begin() const { return m_chatItems.begin(); }
    ChatModelIterator end() const { return m_chatItems.end(); }
    void lock() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }

    bool serialize(QDataStream &stream, int version) const
    {
        QMutexLocker locker(&m_mutex);
        stream << int(m_chatItems.size());
        for (const auto &c : m_chatItems) {
            // FIXME: This 'id' should be eliminated the next time we bump serialization version.
            // (Jared) This was apparently never used.
            int id = 0;
            stream << id;
            stream << c.name;
            stream << c.value;
            stream << c.newResponse;
            stream << c.currentResponse;
            stream << c.stopped;
            stream << c.thumbsUpState;
            stream << c.thumbsDownState;
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
        int size;
        stream >> size;
        for (int i = 0; i < size; ++i) {
            ChatItem c;
            // FIXME: see comment in serialization about id
            int id;
            stream >> id;
            stream >> c.name;
            stream >> c.value;
            if (version < 10) {
                // This is deprecated and no longer used
                QString prompt;
                stream >> prompt;
            }
            stream >> c.newResponse;
            stream >> c.currentResponse;
            stream >> c.stopped;
            stream >> c.thumbsUpState;
            stream >> c.thumbsDownState;
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
            m_mutex.lock();
            const int count = m_chatItems.size();
            m_mutex.unlock();
            beginInsertRows(QModelIndex(), count, count);
            {
                QMutexLocker locker(&m_mutex);
                m_chatItems.append(c);
            }
            endInsertRows();
        }
        emit countChanged();
        return stream.status() == QDataStream::Ok;
    }

Q_SIGNALS:
    void countChanged();
    void valueChanged(int index, const QString &value);

private:
    mutable QMutex m_mutex;
    QList<ChatItem> m_chatItems;
};

#endif // CHATMODEL_H
