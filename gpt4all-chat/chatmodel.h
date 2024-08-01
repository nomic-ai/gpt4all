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
    Q_PROPERTY(QList<SourceExcerpt> sources MEMBER sources)

public:
    // TODO: Maybe we should include the model name here as well as timestamp?
    int id = 0;
    QString name;
    QString value;
    QString prompt;
    QString newResponse;
    QList<SourceExcerpt> sources;
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
        SourcesRole
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

    Q_INVOKABLE void updateSources(int index, const QList<SourceExcerpt> &sources)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (sources.isEmpty()) {
            item.sources.clear();
        } else {
            item.sources << sources;
        }
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {SourcesRole});
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
            stream << SourceExcerpt::toJson(c.sources);
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
            if (version > 9) {
                QList<SourceExcerpt> sources;
                QString json;
                stream >> json;
                QString errorString;
                sources = SourceExcerpt::fromJson(json, errorString);
                Q_ASSERT(errorString.isEmpty());
                c.sources = sources;
            } else if (version > 7) {
                qsizetype count;
                stream >> count;
                QList<SourceExcerpt> sources;
                for (int i = 0; i < count; ++i) {
                    SourceExcerpt source;
                    stream >> source.collection;
                    stream >> source.path;
                    stream >> source.file;
                    stream >> source.title;
                    stream >> source.author;
                    stream >> source.date;
                    Excerpt excerpt;
                    stream >> excerpt.text;
                    stream >> excerpt.page;
                    stream >> excerpt.from;
                    stream >> excerpt.to;
                    source.excerpts = QList{ excerpt };
                    sources.append(source);
                }
                c.sources = sources;
            } else if (version > 2) {
                QString references;
                QList<QString> referencesContext;
                stream >> references;
                stream >> referencesContext;

                if (!references.isEmpty()) {
                    QList<SourceExcerpt> sources;
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
                        SourceExcerpt source;
                        Excerpt excerpt;
                        QTextStream refStream(&reference);
                        QString dummy;
                        int validReferenceNumber;
                        refStream >> validReferenceNumber >> dummy;
                        // Extract title (between quotes)
                        if (reference.contains("\"")) {
                            int startIndex = reference.indexOf('"') + 1;
                            int endIndex = reference.indexOf('"', startIndex);
                            source.title = reference.mid(startIndex, endIndex - startIndex);
                        }

                        // Extract author (after "By " and before the next period)
                        if (reference.contains("By ")) {
                            int startIndex = reference.indexOf("By ") + 3;
                            int endIndex = reference.indexOf('.', startIndex);
                            source.author = reference.mid(startIndex, endIndex - startIndex).trimmed();
                        }

                        // Extract date (after "Date: " and before the next period)
                        if (reference.contains("Date: ")) {
                            int startIndex = reference.indexOf("Date: ") + 6;
                            int endIndex = reference.indexOf('.', startIndex);
                            source.date = reference.mid(startIndex, endIndex - startIndex).trimmed();
                        }

                        // Extract file name (after "In " and before the "[Context]")
                        if (reference.contains("In ") && reference.contains(". [Context]")) {
                            int startIndex = reference.indexOf("In ") + 3;
                            int endIndex = reference.indexOf(". [Context]", startIndex);
                            source.file = reference.mid(startIndex, endIndex - startIndex).trimmed();
                        }

                        // Extract page number (after "Page " and before the next space)
                        if (reference.contains("Page ")) {
                            int startIndex = reference.indexOf("Page ") + 5;
                            int endIndex = reference.indexOf(' ', startIndex);
                            if (endIndex == -1) endIndex = reference.length();
                            excerpt.page = reference.mid(startIndex, endIndex - startIndex).toInt();
                        }

                        // Extract lines (after "Lines " and before the next space or hyphen)
                        if (reference.contains("Lines ")) {
                            int startIndex = reference.indexOf("Lines ") + 6;
                            int endIndex = reference.indexOf(' ', startIndex);
                            if (endIndex == -1) endIndex = reference.length();
                            int hyphenIndex = reference.indexOf('-', startIndex);
                            if (hyphenIndex != -1 && hyphenIndex < endIndex) {
                                excerpt.from = reference.mid(startIndex, hyphenIndex - startIndex).toInt();
                                excerpt.to = reference.mid(hyphenIndex + 1, endIndex - hyphenIndex - 1).toInt();
                            } else {
                                excerpt.from = reference.mid(startIndex, endIndex - startIndex).toInt();
                            }
                        }
                        excerpt.text = context;
                        source.excerpts = QList{ excerpt };
                        sources.append(source);
                    }

                    c.sources = sources;
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
