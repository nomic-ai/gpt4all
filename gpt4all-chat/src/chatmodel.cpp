#include "chatmodel.h"

QList<ResultInfo> ChatItem::consolidateSources(const QList<ResultInfo> &sources)
{
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

void ChatItem::serialize(QDataStream &stream, int version)
{
    // FIXME: This 'id' should be eliminated the next time we bump serialization version.
    // (Jared) This was apparently never used.
    int id = 0;
    stream << id;
    stream << name;
    stream << value;
    stream << newResponse;
    stream << isCurrentResponse;
    stream << stopped;
    stream << thumbsUpState;
    stream << thumbsDownState;
    if (version >= 11 && type() == ChatItem::Type::Response)
        stream << isError;
    if (version >= 8) {
        stream << sources.size();
        for (const ResultInfo &info : sources) {
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
        for (const ResultInfo &info : sources) {
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
        stream << promptAttachments.size();
        for (const PromptAttachment &a : promptAttachments) {
            Q_ASSERT(!a.url.isEmpty());
            stream << a.url;
            stream << a.content;
        }
    }
}

bool ChatItem::deserialize(QDataStream &stream, int version)
{
    // FIXME: see comment in serialization about id
    int id;
    stream >> id;
    stream >> name;
    try {
        type(); // check name
    } catch (const std::exception &e) {
        qWarning() << "ChatModel ERROR:" << e.what();
        return false;
    }
    stream >> value;
    if (version < 10) {
        // This is deprecated and no longer used
        QString prompt;
        stream >> prompt;
    }
    stream >> newResponse;
    stream >> isCurrentResponse;
    stream >> stopped;
    stream >> thumbsUpState;
    stream >> thumbsDownState;
    if (version >= 11 && type() == ChatItem::Type::Response)
        stream >> isError;
    if (version >= 8) {
        qsizetype count;
        stream >> count;
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
        consolidatedSources = ChatItem::consolidateSources(sources);
    } else if (version >= 3) {
        QString references;
        QList<QString> referencesContext;
        stream >> references;
        stream >> referencesContext;

        if (!references.isEmpty()) {
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

            consolidatedSources = ChatItem::consolidateSources(sources);
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
        promptAttachments = attachments;
    }
    return true;
}
