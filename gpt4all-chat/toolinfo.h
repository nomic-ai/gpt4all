#ifndef SOURCEEXCERT_H
#define SOURCEEXCERT_H

#include <QObject>
#include <QJsonObject>
#include <QFileInfo>
#include <QUrl>

using namespace Qt::Literals::StringLiterals;

struct SourceExcerpt {
    Q_GADGET
    Q_PROPERTY(QString date MEMBER date)
    Q_PROPERTY(QString text MEMBER text)
    Q_PROPERTY(QString collection MEMBER collection)
    Q_PROPERTY(QString path MEMBER path)
    Q_PROPERTY(QString file MEMBER file)
    Q_PROPERTY(QString url MEMBER url)
    Q_PROPERTY(QString favicon MEMBER favicon)
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString author MEMBER author)
    Q_PROPERTY(int page MEMBER page)
    Q_PROPERTY(int from MEMBER from)
    Q_PROPERTY(int to MEMBER to)
    Q_PROPERTY(QString fileUri READ fileUri STORED false)

public:
    QString date;       // [Required] The creation or the last modification date whichever is latest
    QString text;       // [Required] The text actually used in the augmented context
    QString collection; // [Optional] The name of the collection
    QString path;       // [Optional] The full path
    QString file;       // [Optional] The name of the file, but not the full path
    QString url;        // [Optional] The name of the remote url
    QString favicon;    // [Optional] The favicon
    QString title;      // [Optional] The title of the document
    QString author;     // [Optional] The author of the document
    int page = -1;      // [Optional] The page where the text was found
    int from = -1;      // [Optional] The line number where the text begins
    int to = -1;        // [Optional] The line number where the text ends

    QString fileUri() const {
        // QUrl reserved chars that are not UNSAFE_PATH according to glib/gconvert.c
        static const QByteArray s_exclude = "!$&'()*+,/:=@~"_ba;

        Q_ASSERT(!QFileInfo(path).isRelative());
#ifdef Q_OS_WINDOWS
        Q_ASSERT(!path.contains('\\')); // Qt normally uses forward slash as path separator
#endif

        auto escaped = QString::fromUtf8(QUrl::toPercentEncoding(path, s_exclude));
        if (escaped.front() != '/')
            escaped = '/' + escaped;
        return u"file://"_s + escaped;
    }

    QJsonObject toJson() const
    {
        QJsonObject result;
        result.insert("date", date);
        result.insert("text", text);
        result.insert("collection", collection);
        result.insert("path", path);
        result.insert("file", file);
        result.insert("url", url);
        result.insert("favicon", favicon);
        result.insert("title", title);
        result.insert("author", author);
        result.insert("page", page);
        result.insert("from", from);
        result.insert("to", to);
        return result;
    }

    bool operator==(const SourceExcerpt &other) const {
        return date == other.date &&
               text == other.text &&
               collection == other.collection &&
               path == other.path &&
               file == other.file &&
               url == other.url &&
               favicon == other.favicon &&
               title == other.title &&
               author == other.author &&
               page == other.page &&
               from == other.from &&
               to == other.to;
    }
    bool operator!=(const SourceExcerpt &other) const {
        return !(*this == other);
    }
};

Q_DECLARE_METATYPE(SourceExcerpt)

#endif // SOURCEEXCERT_H
