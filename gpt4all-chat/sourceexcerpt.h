#ifndef SOURCEEXCERT_H
#define SOURCEEXCERT_H

#include <QObject>
#include <QJsonObject>
#include <QFileInfo>
#include <QUrl>

using namespace Qt::Literals::StringLiterals;

struct Excerpt {
    QString text;       // [Required] The text actually used in the augmented context
    int page = -1;      // [Optional] The page where the text was found
    int from = -1;      // [Optional] The line number where the text begins
    int to = -1;        // [Optional] The line number where the text ends
    bool operator==(const Excerpt &other) const {
        return text == other.text && page == other.page && from == other.from && to == other.to;
    }
    bool operator!=(const Excerpt &other) const {
        return !(*this == other);
    }
};
Q_DECLARE_METATYPE(Excerpt)

struct SourceExcerpt {
    Q_GADGET
    Q_PROPERTY(QString date MEMBER date)
    Q_PROPERTY(QString collection MEMBER collection)
    Q_PROPERTY(QString path MEMBER path)
    Q_PROPERTY(QString file MEMBER file)
    Q_PROPERTY(QString url MEMBER url)
    Q_PROPERTY(QString favicon MEMBER favicon)
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString author MEMBER author)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString fileUri READ fileUri STORED false)
    Q_PROPERTY(QString text READ text STORED false)
    Q_PROPERTY(QList<Excerpt> excerpts MEMBER excerpts)

public:
    QString date;           // [Required] The creation or the last modification date whichever is latest
    QString collection;     // [Optional] The name of the collection
    QString path;           // [Optional] The full path
    QString file;           // [Optional] The name of the file, but not the full path
    QString url;            // [Optional] The name of the remote url
    QString favicon;        // [Optional] The favicon
    QString title;          // [Optional] The title of the document
    QString author;         // [Optional] The author of the document
    QString description;    // [Optional] The description of the source
    QList<Excerpt> excerpts;// [Required] The list of excerpts

    // Returns a human readable string containing all the excerpts
    QString text() const {
        QStringList formattedExcerpts;
        for (const auto& excerpt : excerpts) {
            QString formattedExcerpt = excerpt.text;
            if (excerpt.page != -1) {
                formattedExcerpt += QStringLiteral(" (Page: %1").arg(excerpt.page);
                if (excerpt.from != -1 && excerpt.to != -1) {
                    formattedExcerpt += QStringLiteral(", Lines: %1-%2").arg(excerpt.from).arg(excerpt.to);
                }
                formattedExcerpt += QStringLiteral(")");
            } else if (excerpt.from != -1 && excerpt.to != -1) {
                formattedExcerpt += QStringLiteral(" (Lines: %1-%2)").arg(excerpt.from).arg(excerpt.to);
            }
            formattedExcerpts.append(formattedExcerpt);
        }
        return formattedExcerpts.join(QStringLiteral("\n---\n"));
    }

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

    static QString toJson(const QList<SourceExcerpt> &sources);
    static QList<SourceExcerpt> fromJson(const QString &json, QString &errorString);

    bool operator==(const SourceExcerpt &other) const {
        return file == other.file || url == other.url;
    }
    bool operator!=(const SourceExcerpt &other) const {
        return !(*this == other);
    }
};

Q_DECLARE_METATYPE(SourceExcerpt)

#endif // SOURCEEXCERT_H
