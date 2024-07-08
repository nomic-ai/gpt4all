#ifndef CHATMODEL_H
#define CHATMODEL_H

#include "database.h"

#include <QAbstractListModel>
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

struct ChatItem
{
    Q_GADGET
    Q_PROPERTY(int id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString value MEMBER value)
    Q_PROPERTY(QString prompt MEMBER prompt)
    Q_PROPERTY(QString newResponse MEMBER newResponse)
    Q_PROPERTY(bool currentResponse MEMBER currentResponse)
    Q_PROPERTY(bool stopped MEMBER stopped)
    Q_PROPERTY(bool thumbsUpState MEMBER thumbsUpState)
    Q_PROPERTY(bool thumbsDownState MEMBER thumbsDownState)
    Q_PROPERTY(QList<ResultInfo> sources MEMBER sources)
    Q_PROPERTY(QList<ResultInfo> consolidatedSources MEMBER consolidatedSources)

public:
    // TODO: Maybe we should include the model name here as well as timestamp?
    int id = 0;
    QString name;
    QString value;
    QString prompt;
    QString newResponse;
    QList<ResultInfo> sources;
    QList<ResultInfo> consolidatedSources;
    bool currentResponse = false;
    bool stopped = false;
    bool thumbsUpState = false;
    bool thumbsDownState = false;
};
Q_DECLARE_METATYPE(ChatItem)

class ChatModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit ChatModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ValueRole,
        PromptRole,
        NewResponseRole,
        CurrentResponseRole,
        StoppedRole,
        ThumbsUpStateRole,
        ThumbsDownStateRole,
        SourcesRole,
        ConsolidatedSourcesRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return m_chatItems.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_chatItems.size())
            return QVariant();

        const ChatItem &item = m_chatItems.at(index.row());
        switch (role) {
            case IdRole:
                return item.id;
            case NameRole:
                return item.name;
            case ValueRole:
                return item.value;
            case PromptRole:
                return item.prompt;
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
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[IdRole] = "id";
        roles[NameRole] = "name";
        roles[ValueRole] = "value";
        roles[PromptRole] = "prompt";
        roles[NewResponseRole] = "newResponse";
        roles[CurrentResponseRole] = "currentResponse";
        roles[StoppedRole] = "stopped";
        roles[ThumbsUpStateRole] = "thumbsUpState";
        roles[ThumbsDownStateRole] = "thumbsDownState";
        roles[SourcesRole] = "sources";
        roles[ConsolidatedSourcesRole] = "consolidatedSources";
        return roles;
    }

    void appendPrompt(const QString &name, const QString &value)
    {
        ChatItem item;
        item.name = name;
        item.value = value;
        beginInsertRows(QModelIndex(), m_chatItems.size(), m_chatItems.size());
        m_chatItems.append(item);
        endInsertRows();
        emit countChanged();
    }

    void appendResponse(const QString &name, const QString &prompt)
    {
        ChatItem item;
        item.id = m_chatItems.count(); // This is only relevant for responses
        item.name = name;
        item.prompt = prompt;
        item.currentResponse = true;
        beginInsertRows(QModelIndex(), m_chatItems.size(), m_chatItems.size());
        m_chatItems.append(item);
        endInsertRows();
        emit countChanged();
    }

    Q_INVOKABLE void clear()
    {
        if (m_chatItems.isEmpty()) return;

        beginResetModel();
        m_chatItems.clear();
        endResetModel();
        emit countChanged();
    }

    Q_INVOKABLE ChatItem get(int index)
    {
        if (index < 0 || index >= m_chatItems.size()) return ChatItem();
        return m_chatItems.at(index);
    }

    Q_INVOKABLE void updateCurrentResponse(int index, bool b)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.currentResponse != b) {
            item.currentResponse = b;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {CurrentResponseRole});
        }
    }

    Q_INVOKABLE void updateStopped(int index, bool b)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.stopped != b) {
            item.stopped = b;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {StoppedRole});
        }
    }

    Q_INVOKABLE void updateValue(int index, const QString &value)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.value != value) {
            item.value = value;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ValueRole});
            emit valueChanged(index, value);
        }
    }

    QList<ResultInfo> consolidateSources(const QList<ResultInfo> &sources) {
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
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        item.sources = sources;
        item.consolidatedSources = consolidateSources(sources);
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {SourcesRole});
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ConsolidatedSourcesRole});
    }

    Q_INVOKABLE void updateThumbsUpState(int index, bool b)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.thumbsUpState != b) {
            item.thumbsUpState = b;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ThumbsUpStateRole});
        }
    }

    Q_INVOKABLE void updateThumbsDownState(int index, bool b)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.thumbsDownState != b) {
            item.thumbsDownState = b;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ThumbsDownStateRole});
        }
    }

    Q_INVOKABLE void updateNewResponse(int index, const QString &newResponse)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.newResponse != newResponse) {
            item.newResponse = newResponse;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {NewResponseRole});
        }
    }

    int count() const { return m_chatItems.size(); }

    bool serialize(QDataStream &stream, int version) const
    {
        stream << count();
        for (const auto &c : m_chatItems) {
            stream << c.id;
            stream << c.name;
            stream << c.value;
            stream << c.prompt;
            stream << c.newResponse;
            stream << c.currentResponse;
            stream << c.stopped;
            stream << c.thumbsUpState;
            stream << c.thumbsDownState;
            if (version > 7) {
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
            } else if (version > 2) {
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
        }
        return stream.status() == QDataStream::Ok;
    }

    bool deserialize(QDataStream &stream, int version)
    {
        int size;
        stream >> size;
        for (int i = 0; i < size; ++i) {
            ChatItem c;
            stream >> c.id;
            stream >> c.name;
            stream >> c.value;
            stream >> c.prompt;
            stream >> c.newResponse;
            stream >> c.currentResponse;
            stream >> c.stopped;
            stream >> c.thumbsUpState;
            stream >> c.thumbsDownState;
            if (version > 7) {
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
            }else if (version > 2) {
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
            beginInsertRows(QModelIndex(), m_chatItems.size(), m_chatItems.size());
            m_chatItems.append(c);
            endInsertRows();
        }
        emit countChanged();
        return stream.status() == QDataStream::Ok;
    }

    QVector<QPair<QString, QString>> text() const
    {
        QVector<QPair<QString, QString>> result;
        for (const auto &c : m_chatItems)
            result << qMakePair(c.name, c.value);
        return result;
    }

Q_SIGNALS:
    void countChanged();
    void valueChanged(int index, const QString &value);

private:

    QList<ChatItem> m_chatItems;
};

#endif // CHATMODEL_H
