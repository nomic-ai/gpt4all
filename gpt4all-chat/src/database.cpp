#include "database.h"

#include "mysettings.h"
#include "utils.h"

#include <duckx/duckx.hpp>
#include <fmt/format.h>
#include <usearch/index_plugins.hpp>

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileSystemWatcher>
#include <QIODevice>
#include <QPdfDocument>
#include <QPdfSelection>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>
#include <QTimer>
#include <QMap>
#include <QUtf8StringView>
#include <QVariant>
#include <Qt>
#include <QtLogging>

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

using namespace Qt::Literals::StringLiterals;
namespace ranges = std::ranges;
namespace us = unum::usearch;

//#define DEBUG
//#define DEBUG_EXAMPLE

namespace {

/* QFile that checks input for binary data. If seen, it fails the read and returns true
 * for binarySeen(). */
class BinaryDetectingFile: public QFile {
public:
    using QFile::QFile;

    bool binarySeen() const { return m_binarySeen; }

protected:
    qint64 readData(char *data, qint64 maxSize) override {
        qint64 res = QFile::readData(data, maxSize);
        return checkData(data, res);
    }

    qint64 readLineData(char *data, qint64 maxSize) override {
        qint64 res = QFile::readLineData(data, maxSize);
        return checkData(data, res);
    }

private:
    qint64 checkData(const char *data, qint64 size) {
        Q_ASSERT(!isTextModeEnabled()); // We need raw bytes from the underlying QFile
        if (size != -1 && !m_binarySeen) {
            for (qint64 i = 0; i < size; i++) {
                /* Control characters we should never see in plain text:
                 * 0x00 NUL - 0x06 ACK
                 * 0x0E SO  - 0x1A SUB
                 * 0x1C FS  - 0x1F US */
                auto c = static_cast<unsigned char>(data[i]);
                if (c < 0x07 || (c >= 0x0E && c < 0x1B) || (c >= 0x1C && c < 0x20)) {
                    m_binarySeen = true;
                    break;
                }
            }
        }
        return m_binarySeen ? -1 : size;
    }

    bool m_binarySeen = false;
};

} // namespace

static int s_batchSize = 100;

static const QString INIT_DB_SQL[] = {
    // automatically free unused disk space
    u"pragma auto_vacuum = FULL;"_s,
    // create tables
    uR"(
        create table chunks(
            id            integer primary key autoincrement,
            document_id   integer not null,
            chunk_text    text not null,
            file          text not null,
            title         text,
            author        text,
            subject       text,
            keywords      text,
            page          integer,
            line_from     integer,
            line_to       integer,
            words         integer default 0 not null,
            tokens        integer default 0 not null,
            foreign key(document_id) references documents(id)
        );
    )"_s, uR"(
        create virtual table chunks_fts using fts5(
            id unindexed,
            document_id unindexed,
            chunk_text,
            file,
            title,
            author,
            subject,
            keywords,
            content='chunks',
            content_rowid='id',
            tokenize='porter'
        );
    )"_s, uR"(
        create table collections(
            id                  integer primary key,
            name                text unique not null,
            start_update_time   integer,
            last_update_time    integer,
            embedding_model     text
        );
    )"_s, uR"(
        create table folders(
            id   integer primary key autoincrement,
            path text unique not null
        );
    )"_s, uR"(
        create table collection_items(
            collection_id integer not null,
            folder_id     integer not null,
            foreign key(collection_id) references collections(id)
            foreign key(folder_id)     references folders(id),
            unique(collection_id, folder_id)
        );
    )"_s, uR"(
        create table documents(
            id            integer primary key,
            folder_id     integer not null,
            document_time integer not null,
            document_path text unique not null,
            foreign key(folder_id) references folders(id)
        );
    )"_s, uR"(
        create table embeddings(
            model         text not null,
            folder_id     integer not null,
            chunk_id      integer not null,
            embedding     blob not null,
            primary key(model, folder_id, chunk_id),
            foreign key(folder_id) references folders(id),
            foreign key(chunk_id)  references chunks(id),
            unique(model, chunk_id)
        );
    )"_s,
};

static const QString INSERT_CHUNK_SQL = uR"(
    insert into chunks(document_id, chunk_text,
        file, title, author, subject, keywords, page, line_from, line_to, words)
        values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        returning id;
)"_s;

static const QString INSERT_CHUNK_FTS_SQL = uR"(
        insert into chunks_fts(document_id, chunk_text,
            file, title, author, subject, keywords)
            values(?, ?, ?, ?, ?, ?, ?);
)"_s;

static const QString SELECT_CHUNKED_DOCUMENTS_SQL[] = {
    uR"(
        select distinct document_id from chunks;
    )"_s, uR"(
        select distinct document_id from chunks_fts;
    )"_s,
};

static const QString DELETE_CHUNKS_SQL[] = {
    uR"(
        delete from embeddings
        where chunk_id in (
            select id from chunks where document_id = ?
        );
    )"_s, uR"(
        delete from chunks where document_id = ?;
    )"_s, uR"(
        delete from chunks_fts where document_id = ?;
    )"_s,
};

static const QString SELECT_CHUNKS_BY_DOCUMENT_SQL = uR"(
    select id from chunks WHERE document_id = ?;
)"_s;

static const QString SELECT_CHUNKS_SQL = uR"(
    select c.id, d.document_time, d.document_path, c.chunk_text, c.file, c.title, c.author, c.page, c.line_from, c.line_to, co.name
    from chunks c
    join documents d on d.id = c.document_id
    join folders f on f.id = d.folder_id
    join collection_items ci on ci.folder_id = f.id
    join collections co on co.id = ci.collection_id
    where c.id in (%1);
)"_s;

static const QString SELECT_UNCOMPLETED_CHUNKS_SQL = uR"(
    select co.name, co.embedding_model, c.id, d.folder_id, c.chunk_text
    from chunks c
    join documents d on d.id = c.document_id
    join folders f on f.id = d.folder_id
    join collection_items ci on ci.folder_id = f.id
    join collections co on co.id = ci.collection_id and co.embedding_model is not null
    where not exists(
        select 1
        from embeddings e
        where e.chunk_id = c.id and e.model = co.embedding_model
    );
)"_s;

static const QString SELECT_COUNT_CHUNKS_SQL = uR"(
    select count(c.id)
    from chunks c
    join documents d on d.id = c.document_id
    where d.folder_id = ?;
)"_s;

static const QString SELECT_CHUNKS_FTS_SQL = uR"(
    select fts.id, bm25(chunks_fts) as score
    from chunks_fts fts
    join documents d on fts.document_id = d.id
    join collection_items ci on d.folder_id = ci.folder_id
    join collections co on ci.collection_id = co.id
    where chunks_fts match ?
    and co.name in ('%1')
    order by score limit %2;
)"_s;


#define NAMED_PAIR(name, typea, a, typeb, b) \
    struct name { typea a; typeb b; }; \
    static bool operator==(const name &x, const name &y) { return x.a == y.a && x.b == y.b; } \
    static size_t qHash(const name &x, size_t seed) { return qHashMulti(seed, x.a, x.b); }

// struct compared by embedding key, can be extended with additional unique data
NAMED_PAIR(EmbeddingKey, QString, embedding_model, int, chunk_id)

namespace {
    struct IncompleteChunk: EmbeddingKey { int folder_id; QString text; };
} // namespace

static bool selectAllUncompletedChunks(QSqlQuery &q, QHash<IncompleteChunk, QStringList> &chunks)
{
    if (!q.exec(SELECT_UNCOMPLETED_CHUNKS_SQL))
        return false;
    while (q.next()) {
        QString collection = q.value(0).toString();
        IncompleteChunk ic {
            /*EmbeddingKey*/ {
                .embedding_model = q.value(1).toString(),
                .chunk_id        = q.value(2).toInt(),
            },
            /*folder_id =*/ q.value(3).toInt(),
            /*text      =*/ q.value(4).toString(),
        };
        chunks[ic] << collection;
    }
    return true;
}

static bool selectCountChunks(QSqlQuery &q, int folder_id, int &count)
{
    if (!q.prepare(SELECT_COUNT_CHUNKS_SQL))
        return false;
    q.addBindValue(folder_id);
    if (!q.exec())
        return false;
    if (!q.next()) {
        count = 0;
        return false;
    }
    count = q.value(0).toInt();
    return true;
}

static bool selectChunk(QSqlQuery &q, const QList<int> &chunk_ids)
{
    QString chunk_ids_str = QString::number(chunk_ids[0]);
    for (size_t i = 1; i < chunk_ids.size(); ++i)
        chunk_ids_str += "," + QString::number(chunk_ids[i]);
    const QString formatted_query = SELECT_CHUNKS_SQL.arg(chunk_ids_str);
    if (!q.prepare(formatted_query))
        return false;
    return q.exec();
}

static const QString INSERT_COLLECTION_SQL = uR"(
    insert into collections(name, start_update_time, last_update_time, embedding_model)
        values(?, ?, ?, ?)
        returning id;
    )"_s;

static const QString SELECT_FOLDERS_FROM_COLLECTIONS_SQL = uR"(
    select f.id, f.path
    from collections c
    join collection_items ci on ci.collection_id = c.id
    join folders f on ci.folder_id = f.id
    where c.name = ?;
    )"_s;

static const QString SELECT_COLLECTIONS_SQL_V1 = uR"(
    select c.collection_name, f.folder_path, f.id
    from collections c
    join folders f on c.folder_id = f.id
    order by c.collection_name asc, f.folder_path asc;
    )"_s;

static const QString SELECT_COLLECTIONS_SQL_V2 = uR"(
    select c.id, c.name, f.path, f.id, c.start_update_time, c.last_update_time, c.embedding_model
    from collections c
    join collection_items ci on ci.collection_id = c.id
    join folders f on ci.folder_id = f.id
    order by c.name asc, f.path asc;
    )"_s;

static const QString SELECT_COLLECTION_BY_NAME_SQL = uR"(
    select id, name, start_update_time, last_update_time, embedding_model
    from collections c
    where name = ?;
    )"_s;

static const QString SET_COLLECTION_EMBEDDING_MODEL_SQL = uR"(
    update collections
    set embedding_model = ?
    where name = ?;
    )"_s;

static const QString UPDATE_START_UPDATE_TIME_SQL = uR"(
    update collections set start_update_time = ? where id = ?;
)"_s;

static const QString UPDATE_LAST_UPDATE_TIME_SQL = uR"(
    update collections set last_update_time = ? where id = ?;
)"_s;

static const QString FTS_INTEGRITY_SQL = uR"(
    insert into chunks_fts(chunks_fts, rank) values('integrity-check', 1);
)"_s;

static const QString FTS_REBUILD_SQL = uR"(
    insert into chunks_fts(chunks_fts) values('rebuild');
)"_s;

static bool addCollection(QSqlQuery &q, const QString &collection_name, const QDateTime &start_update,
                          const QDateTime &last_update, const QString &embedding_model, CollectionItem &item)
{
    if (!q.prepare(INSERT_COLLECTION_SQL))
        return false;
    q.addBindValue(collection_name);
    q.addBindValue(start_update);
    q.addBindValue(last_update);
    q.addBindValue(embedding_model);
    if (!q.exec() || !q.next())
        return false;
    item.collection_id = q.value(0).toInt();
    item.collection = collection_name;
    item.embeddingModel = embedding_model;
    return true;
}

static bool selectFoldersFromCollection(QSqlQuery &q, const QString &collection_name, QList<QPair<int, QString>> *folders)
{
    if (!q.prepare(SELECT_FOLDERS_FROM_COLLECTIONS_SQL))
        return false;
    q.addBindValue(collection_name);
    if (!q.exec())
        return false;
    while (q.next())
        folders->append({q.value(0).toInt(), q.value(1).toString()});
    return true;
}

static QList<CollectionItem> sqlExtractCollections(QSqlQuery &q, bool with_folder = false, int version = LOCALDOCS_VERSION)
{
    QList<CollectionItem> collections;
    while (q.next()) {
        CollectionItem i;
        int idx = 0;
        if (version >= 2)
            i.collection_id = q.value(idx++).toInt();
        i.collection = q.value(idx++).toString();
        if (with_folder) {
            i.folder_path = q.value(idx++).toString();
            i.folder_id = q.value(idx++).toInt();
        }
        i.indexing = false;
        i.installed = true;

        if (version >= 2) {
            bool ok;
            const qint64 start_update = q.value(idx++).toLongLong(&ok);
            if (ok) i.startUpdate = QDateTime::fromMSecsSinceEpoch(start_update);
            const qint64 last_update = q.value(idx++).toLongLong(&ok);
            if (ok) i.lastUpdate = QDateTime::fromMSecsSinceEpoch(last_update);

            i.embeddingModel = q.value(idx++).toString();
        }
        if (i.embeddingModel.isNull()) {
            // unknown embedding model -> need to re-index
            i.forceIndexing = true;
        }

        collections << i;
    }
    return collections;
}

static bool selectAllFromCollections(QSqlQuery &q, QList<CollectionItem> *collections, int version = LOCALDOCS_VERSION)
{

    switch (version) {
    case 1:
        if (!q.prepare(SELECT_COLLECTIONS_SQL_V1))
            return false;
        break;
    case 2:
    case 3:
        if (!q.prepare(SELECT_COLLECTIONS_SQL_V2))
            return false;
        break;
    default:
        Q_UNREACHABLE();
        return false;
    }

    if (!q.exec())
        return false;
    *collections = sqlExtractCollections(q, true, version);
    return true;
}

static bool selectCollectionByName(QSqlQuery &q, const QString &name, std::optional<CollectionItem> &collection)
{
    if (!q.prepare(SELECT_COLLECTION_BY_NAME_SQL))
        return false;
    q.addBindValue(name);
    if (!q.exec())
        return false;
    QList<CollectionItem> collections = sqlExtractCollections(q);
    Q_ASSERT(collections.count() <= 1);
    collection.reset();
    if (!collections.isEmpty())
        collection = collections.first();
    return true;
}

static bool setCollectionEmbeddingModel(QSqlQuery &q, const QString &collection_name, const QString &embedding_model)
{
    if (!q.prepare(SET_COLLECTION_EMBEDDING_MODEL_SQL))
        return false;
    q.addBindValue(embedding_model);
    q.addBindValue(collection_name);
    return q.exec();
}

static bool updateStartUpdateTime(QSqlQuery &q, int id, qint64 update_time)
{
    if (!q.prepare(UPDATE_START_UPDATE_TIME_SQL))
        return false;
    q.addBindValue(update_time);
    q.addBindValue(id);
    return q.exec();
}

static bool updateLastUpdateTime(QSqlQuery &q, int id, qint64 update_time)
{
    if (!q.prepare(UPDATE_LAST_UPDATE_TIME_SQL))
        return false;
    q.addBindValue(update_time);
    q.addBindValue(id);
    return q.exec();
}

static const QString INSERT_FOLDERS_SQL = uR"(
    insert into folders(path) values(?);
    )"_s;

static const QString DELETE_FOLDERS_SQL = uR"(
    delete from folders where id = ?;
    )"_s;

static const QString SELECT_FOLDERS_FROM_PATH_SQL = uR"(
    select id from folders where path = ?;
    )"_s;

static const QString GET_FOLDER_EMBEDDING_MODEL_SQL = uR"(
    select co.embedding_model
    from collections co
    join collection_items ci on ci.collection_id = co.id
    where ci.folder_id = ?;
    )"_s;

static const QString FOLDER_REMOVE_ALL_DOCS_SQL[] = {
    uR"(
        delete from embeddings
        where chunk_id in (
            select c.id
            from chunks c
            join documents d on d.id = c.document_id
            join folders f on f.id = d.folder_id
            where f.path = ?
        );
    )"_s, uR"(
        delete from chunks
        where document_id in (
            select d.id
            from documents d
            join folders f on f.id = d.folder_id
            where f.path = ?
        );
    )"_s, uR"(
        delete from documents
        where id in (
            select d.id
            from documents d
            join folders f on f.id = d.folder_id
            where f.path = ?
        );
    )"_s,
};

static bool addFolderToDB(QSqlQuery &q, const QString &folder_path, int *folder_id)
{
    if (!q.prepare(INSERT_FOLDERS_SQL))
        return false;
    q.addBindValue(folder_path);
    if (!q.exec())
        return false;
    *folder_id = q.lastInsertId().toInt();
    return true;
}

static bool removeFolderFromDB(QSqlQuery &q, int folder_id)
{
    if (!q.prepare(DELETE_FOLDERS_SQL))
        return false;
    q.addBindValue(folder_id);
    return q.exec();
}

static bool selectFolder(QSqlQuery &q, const QString &folder_path, int *id)
{
    if (!q.prepare(SELECT_FOLDERS_FROM_PATH_SQL))
        return false;
    q.addBindValue(folder_path);
    if (!q.exec())
        return false;
    Q_ASSERT(q.size() < 2);
    if (q.next())
        *id = q.value(0).toInt();
    return true;
}

static bool sqlGetFolderEmbeddingModel(QSqlQuery &q, int id, QString &embedding_model)
{
    if (!q.prepare(GET_FOLDER_EMBEDDING_MODEL_SQL))
        return false;
    q.addBindValue(id);
    if (!q.exec() || !q.next())
        return false;
    // FIXME(jared): there may be more than one if a folder is shared between collections
    Q_ASSERT(q.size() < 2);
    embedding_model = q.value(0).toString();
    return true;
}

static const QString INSERT_COLLECTION_ITEM_SQL = uR"(
    insert into collection_items(collection_id, folder_id)
    values(?, ?)
    on conflict do nothing;
)"_s;

static const QString DELETE_COLLECTION_FOLDER_SQL = uR"(
    delete from collection_items
    where collection_id = (select id from collections where name = :name) and folder_id = :folder_id
    returning (select count(*) from collection_items where folder_id = :folder_id);
)"_s;

static const QString PRUNE_COLLECTIONS_SQL = uR"(
    delete from collections
    where id not in (select collection_id from collection_items);
)"_s;

// 0 = already exists, 1 = added, -1 = error
static int addCollectionItem(QSqlQuery &q, int collection_id, int folder_id)
{
    if (!q.prepare(INSERT_COLLECTION_ITEM_SQL))
        return -1;
    q.addBindValue(collection_id);
    q.addBindValue(folder_id);
    if (q.exec())
        return q.numRowsAffected();
    return -1;
}

// returns the number of remaining references to the folder, or -1 on error
static int removeCollectionFolder(QSqlQuery &q, const QString &collection_name, int folder_id)
{
    if (!q.prepare(DELETE_COLLECTION_FOLDER_SQL))
        return -1;
    q.bindValue(":name", collection_name);
    q.bindValue(":folder_id", folder_id);
    if (!q.exec() || !q.next())
        return -1;
    return q.value(0).toInt();
}

static bool sqlPruneCollections(QSqlQuery &q)
{
    return q.exec(PRUNE_COLLECTIONS_SQL);
}

static const QString INSERT_DOCUMENTS_SQL = uR"(
    insert into documents(folder_id, document_time, document_path) values(?, ?, ?);
    )"_s;

static const QString UPDATE_DOCUMENT_TIME_SQL = uR"(
    update documents set document_time = ? where id = ?;
    )"_s;

static const QString DELETE_DOCUMENTS_SQL = uR"(
    delete from documents where id = ?;
    )"_s;

static const QString SELECT_DOCUMENT_SQL = uR"(
    select id, document_time from documents where document_path = ?;
    )"_s;

static const QString SELECT_DOCUMENTS_SQL = uR"(
    select id from documents where folder_id = ?;
    )"_s;

static const QString SELECT_ALL_DOCUMENTS_SQL = uR"(
    select id, document_path from documents;
    )"_s;

static const QString SELECT_COUNT_STATISTICS_SQL = uR"(
    select count(distinct d.id), sum(c.words), sum(c.tokens)
    from documents d
    left join chunks c on d.id = c.document_id
    where d.folder_id = ?;
    )"_s;

static bool addDocument(QSqlQuery &q, int folder_id, qint64 document_time, const QString &document_path, int *document_id)
{
    if (!q.prepare(INSERT_DOCUMENTS_SQL))
        return false;
    q.addBindValue(folder_id);
    q.addBindValue(document_time);
    q.addBindValue(document_path);
    if (!q.exec())
        return false;
    *document_id = q.lastInsertId().toInt();
    return true;
}

static bool removeDocument(QSqlQuery &q, int document_id)
{
    if (!q.prepare(DELETE_DOCUMENTS_SQL))
        return false;
    q.addBindValue(document_id);
    return q.exec();
}

static bool updateDocument(QSqlQuery &q, int id, qint64 document_time)
{
    if (!q.prepare(UPDATE_DOCUMENT_TIME_SQL))
        return false;
    q.addBindValue(document_time);
    q.addBindValue(id);
    return q.exec();
}

static bool selectDocument(QSqlQuery &q, const QString &document_path, int *id, qint64 *document_time)
{
    if (!q.prepare(SELECT_DOCUMENT_SQL))
        return false;
    q.addBindValue(document_path);
    if (!q.exec())
        return false;
    Q_ASSERT(q.size() < 2);
    if (q.next()) {
        *id = q.value(0).toInt();
        *document_time = q.value(1).toLongLong();
    }
    return true;
}

static bool selectDocuments(QSqlQuery &q, int folder_id, QList<int> *documentIds)
{
    if (!q.prepare(SELECT_DOCUMENTS_SQL))
        return false;
    q.addBindValue(folder_id);
    if (!q.exec())
        return false;
    while (q.next())
        documentIds->append(q.value(0).toInt());
    return true;
}

static bool selectCountStatistics(QSqlQuery &q, int folder_id, int *total_docs, int *total_words, int *total_tokens)
{
    if (!q.prepare(SELECT_COUNT_STATISTICS_SQL))
        return false;
    q.addBindValue(folder_id);
    if (!q.exec())
        return false;
    if (q.next()) {
        *total_docs = q.value(0).toInt();
        *total_words = q.value(1).toInt();
        *total_tokens = q.value(2).toInt();
    }
    return true;
}

// insert embedding only if still needed
static const QString INSERT_EMBEDDING_SQL = uR"(
    insert into embeddings(model, folder_id, chunk_id, embedding)
    select :model, d.folder_id, :chunk_id, :embedding
    from chunks c
    join documents d on d.id = c.document_id
    join folders f on f.id = d.folder_id
    join collection_items ci on ci.folder_id = f.id
    join collections co on co.id = ci.collection_id
    where co.embedding_model = :model and c.id = :chunk_id
    limit 1;
)"_s;

static const QString GET_COLLECTION_EMBEDDINGS_SQL = uR"(
    select e.chunk_id, e.embedding
    from embeddings e
    join collections co on co.embedding_model = e.model
    join collection_items ci on ci.folder_id = e.folder_id and ci.collection_id = co.id
    where co.name in ('%1');
)"_s;

static const QString GET_CHUNK_EMBEDDINGS_SQL = uR"(
    select e.chunk_id, e.embedding
    from embeddings e
    where e.chunk_id in (%1);
)"_s;

static const QString GET_CHUNK_FILE_SQL = uR"(
    select file from chunks where id = ?;
)"_s;

namespace {
    struct Embedding { QString model; int folder_id; int chunk_id; QByteArray data; };
    struct EmbeddingStat { QString lastFile; int nAdded; int nSkipped; };
} // namespace

NAMED_PAIR(EmbeddingFolder, QString, embedding_model, int, folder_id)

static bool sqlAddEmbeddings(QSqlQuery &q, const QList<Embedding> &embeddings, QHash<EmbeddingFolder, EmbeddingStat> &embeddingStats)
{
    if (!q.prepare(INSERT_EMBEDDING_SQL))
        return false;

    // insert embedding if needed
    for (const auto &e: embeddings) {
        q.bindValue(":model", e.model);
        q.bindValue(":chunk_id", e.chunk_id);
        q.bindValue(":embedding", e.data);
        if (!q.exec())
            return false;

        auto &stat = embeddingStats[{ e.model, e.folder_id }];
        if (q.numRowsAffected()) {
            stat.nAdded++; // embedding added
        } else {
            stat.nSkipped++; // embedding no longer needed
        }
    }

    if (!q.prepare(GET_CHUNK_FILE_SQL))
        return false;

    // populate statistics for each collection item
    for (const auto &e: embeddings) {
        auto &stat = embeddingStats[{ e.model, e.folder_id }];
        if (stat.nAdded && stat.lastFile.isNull()) {
            q.addBindValue(e.chunk_id);
            if (!q.exec() || !q.next())
                return false;
            stat.lastFile = q.value(0).toString();
        }
    }

    return true;
}

void Database::transaction()
{
    bool ok = m_db.transaction();
    Q_ASSERT(ok);
}

void Database::commit()
{
    bool ok = m_db.commit();
    Q_ASSERT(ok);
}

void Database::rollback()
{
    bool ok = m_db.rollback();
    Q_ASSERT(ok);
}

bool Database::refreshDocumentIdCache(QSqlQuery &q)
{
    m_documentIdCache.clear();
    for (const auto &cmd: SELECT_CHUNKED_DOCUMENTS_SQL) {
        if (!q.exec(cmd))
            return false;
        while (q.next())
            m_documentIdCache << q.value(0).toInt();
    }
    return true;
}

bool Database::addChunk(QSqlQuery &q, int document_id, const QString &chunk_text, const QString &file,
                        const QString &title, const QString &author, const QString &subject, const QString &keywords,
                        int page, int from, int to, int words, int *chunk_id)
{
    if (!q.prepare(INSERT_CHUNK_SQL))
        return false;
    q.addBindValue(document_id);
    q.addBindValue(chunk_text);
    q.addBindValue(file);
    q.addBindValue(title);
    q.addBindValue(author);
    q.addBindValue(subject);
    q.addBindValue(keywords);
    q.addBindValue(page);
    q.addBindValue(from);
    q.addBindValue(to);
    q.addBindValue(words);
    if (!q.exec() || !q.next())
        return false;
    *chunk_id = q.value(0).toInt();

    if (!q.prepare(INSERT_CHUNK_FTS_SQL))
        return false;
    q.addBindValue(document_id);
    q.addBindValue(chunk_text);
    q.addBindValue(file);
    q.addBindValue(title);
    q.addBindValue(author);
    q.addBindValue(subject);
    q.addBindValue(keywords);
    if (!q.exec())
        return false;
    m_documentIdCache << document_id;
    return true;
}

bool Database::removeChunksByDocumentId(QSqlQuery &q, int document_id)
{
    for (const auto &cmd: DELETE_CHUNKS_SQL) {
        if (!q.prepare(cmd))
            return false;
        q.addBindValue(document_id);
        if (!q.exec())
            return false;
    }
    m_documentIdCache.remove(document_id);
    return true;
}

bool Database::sqlRemoveDocsByFolderPath(QSqlQuery &q, const QString &path)
{
    for (const auto &cmd: FOLDER_REMOVE_ALL_DOCS_SQL) {
        if (!q.prepare(cmd))
            return false;
        q.addBindValue(path);
        if (!q.exec())
            return false;
    }
    return refreshDocumentIdCache(q);
}

bool Database::hasContent()
{
    return m_db.tables().contains("chunks", Qt::CaseInsensitive);
}

int Database::openDatabase(const QString &modelPath, bool create, int ver)
{
    if (!QFileInfo(modelPath).isDir()) {
        qWarning() << "ERROR: invalid download path" << modelPath;
        return -1;
    }
    if (m_db.isOpen())
        m_db.close();
    auto dbPath = u"%1/localdocs_v%2.db"_s.arg(modelPath).arg(ver);
    if (!create && !QFileInfo::exists(dbPath))
        return 0;
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        qWarning() << "ERROR: opening db" << dbPath << m_db.lastError();
        return -1;
    }
    return hasContent();
}

bool Database::openLatestDb(const QString &modelPath, QList<CollectionItem> &oldCollections)
{
    /*
     * Support upgrade path from older versions:
     *
     *  1. Detect and load dbPath with older versions
     *  2. Provide versioned SQL select statements
     *  3. Upgrade the tables to the new version
     *  4. By default mark all collections of older versions as force indexing and present to the user
     *     the an 'update' button letting them know a breaking change happened and that the collection
     *     will need to be indexed again
     */

    int dbVer;
    for (dbVer = LOCALDOCS_VERSION;; dbVer--) {
        if (dbVer < LOCALDOCS_MIN_VER) return true; // create a new db
        int res = openDatabase(modelPath, false, dbVer);
        if (res == 1) break; // found one with content
        if (res == -1) return false; // error
    }

    if (dbVer == LOCALDOCS_VERSION) return true; // already up-to-date

    // If we're upgrading, then we need to do a select on the current version of the collections table,
    // then create the new one and populate the collections table and mark them as needing forced
    // indexing

#if defined(DEBUG)
    qDebug() << "Older localdocs version found" << dbVer << "upgrade to" << LOCALDOCS_VERSION;
#endif

    // Select the current collections which will be marked to force indexing
    QSqlQuery q(m_db);
    if (!selectAllFromCollections(q, &oldCollections, dbVer)) {
        qWarning() << "ERROR: Could not open select old collections" << q.lastError();
        return false;
    }

    m_db.close();
    return true;
}

bool Database::initDb(const QString &modelPath, const QList<CollectionItem> &oldCollections)
{
    if (!m_db.isOpen()) {
        int res = openDatabase(modelPath);
        if (res == 1) return true; // already populated
        if (res == -1) return false; // error
    } else if (hasContent()) {
        return true; // already populated
    }

    transaction();

    QSqlQuery q(m_db);
    for (const auto &cmd: INIT_DB_SQL) {
        if (!q.exec(cmd)) {
            qWarning() << "ERROR: failed to create tables" << q.lastError();
            rollback();
            return false;
        }
    }

    /* These are collection items that came from an older version of localdocs which
     * require forced indexing that should only be done when the user has explicitly asked
     * for them to be indexed again */
    for (const CollectionItem &item : oldCollections) {
        if (!addFolder(item.collection, item.folder_path, QString())) {
            qWarning() << "ERROR: failed to add previous collections to new database";
            rollback();
            return false;
        }
    }

    commit();
    return true;
}

Database::Database(int chunkSize, QStringList extensions)
    : QObject(nullptr)
    , m_chunkSize(chunkSize)
    , m_scannedFileExtensions(std::move(extensions))
    , m_scanIntervalTimer(new QTimer(this))
    , m_watcher(new QFileSystemWatcher(this))
    , m_embLLM(new EmbeddingLLM)
    , m_databaseValid(true)
    , m_chunkStreamer(this)
{
    m_db = QSqlDatabase::database(QSqlDatabase::defaultConnection, false);
    if (!m_db.isValid())
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    Q_ASSERT(m_db.isValid());

    moveToThread(&m_dbThread);
    m_dbThread.setObjectName("database");
    m_dbThread.start();
}

Database::~Database()
{
    m_dbThread.quit();
    m_dbThread.wait();
    delete m_embLLM;
}

void Database::setStartUpdateTime(CollectionItem &item)
{
    QSqlQuery q(m_db);
    const qint64 update_time = QDateTime::currentMSecsSinceEpoch();
    if (!updateStartUpdateTime(q, item.collection_id, update_time))
        qWarning() << "Database ERROR: failed to set start update time:" << q.lastError();
    else
        item.startUpdate = QDateTime::fromMSecsSinceEpoch(update_time);
}

void Database::setLastUpdateTime(CollectionItem &item)
{
    QSqlQuery q(m_db);
    const qint64 update_time = QDateTime::currentMSecsSinceEpoch();
    if (!updateLastUpdateTime(q, item.collection_id, update_time))
        qWarning() << "Database ERROR: failed to set last update time:" << q.lastError();
    else
        item.lastUpdate = QDateTime::fromMSecsSinceEpoch(update_time);
}

CollectionItem Database::guiCollectionItem(int folder_id) const
{
    Q_ASSERT(m_collectionMap.contains(folder_id));
    return m_collectionMap.value(folder_id);
}

void Database::updateGuiForCollectionItem(const CollectionItem &item)
{
    m_collectionMap.insert(item.folder_id, item);
    emit requestUpdateGuiForCollectionItem(item);
}

void Database::addGuiCollectionItem(const CollectionItem &item)
{
    m_collectionMap.insert(item.folder_id, item);
    emit requestAddGuiCollectionItem(item);
}

void Database::removeGuiFolderById(const QString &collection, int folder_id)
{
    emit requestRemoveGuiFolderById(collection, folder_id);
}

void Database::guiCollectionListUpdated(const QList<CollectionItem> &collectionList)
{
    for (const auto &i : collectionList)
        m_collectionMap.insert(i.folder_id, i);
    emit requestGuiCollectionListUpdated(collectionList);
}

void Database::updateFolderToIndex(int folder_id, size_t countForFolder, bool sendChunks)
{
    CollectionItem item = guiCollectionItem(folder_id);
    item.currentDocsToIndex = countForFolder;
    if (!countForFolder) {
        if (sendChunks && !m_chunkList.isEmpty())
            sendChunkList(); // send any remaining embedding chunks to llm
        item.indexing = false;
        item.installed = true;

        // Set the last update if we are done
        if (item.startUpdate > item.lastUpdate && item.currentEmbeddingsToIndex == 0)
            setLastUpdateTime(item);
    }
    updateGuiForCollectionItem(item);
}

static void handleDocumentError(const QString &errorMessage, int document_id, const QString &document_path,
                                const QSqlError &error)
{
    qWarning() << errorMessage << document_id << document_path << error;
}

class DocumentReader {
public:
    struct Metadata { QString title, author, subject, keywords; };

    static std::unique_ptr<DocumentReader> fromDocument(const DocumentInfo &info);

    const DocumentInfo           &doc     () const { return *m_info; }
    const Metadata               &metadata() const { return m_metadata; }
    const std::optional<QString> &word    () const { return m_word; }
    const std::optional<QString> &nextWord()       { m_word = advance(); return m_word; }
    virtual std::optional<ChunkStreamer::Status> getError() const { return std::nullopt; }
    virtual int page() const { return -1; }

    virtual ~DocumentReader() = default;

protected:
    explicit DocumentReader(const DocumentInfo &info)
        : m_info(&info) {}

    void postInit(Metadata &&metadata = {})
    {
        m_metadata = std::move(metadata);
        m_word = advance();
    }

    virtual std::optional<QString> advance() = 0;

    const DocumentInfo     *m_info;
    Metadata                m_metadata;
    std::optional<QString>  m_word;
};

namespace {

class PdfDocumentReader final : public DocumentReader {
public:
    explicit PdfDocumentReader(const DocumentInfo &info)
        : DocumentReader(info)
    {
        QString path = info.file.canonicalFilePath();
        if (m_doc.load(path) != QPdfDocument::Error::None)
            throw std::runtime_error(fmt::format("Failed to load PDF: {}", path));
        Metadata metadata {
            .title    = m_doc.metaData(QPdfDocument::MetaDataField::Title   ).toString(),
            .author   = m_doc.metaData(QPdfDocument::MetaDataField::Author  ).toString(),
            .subject  = m_doc.metaData(QPdfDocument::MetaDataField::Subject ).toString(),
            .keywords = m_doc.metaData(QPdfDocument::MetaDataField::Keywords).toString(),
        };
        postInit(std::move(metadata));
    }

    int page() const override { return m_currentPage; }

private:
    std::optional<QString> advance() override
    {
        QString word;
        do {
            while (!m_stream || m_stream->atEnd()) {
                if (m_currentPage >= m_doc.pageCount())
                    return std::nullopt;
                m_pageText = m_doc.getAllText(m_currentPage++).text();
                m_stream.emplace(&m_pageText);
            }
            *m_stream >> word;
        } while (word.isEmpty());
        return word;
    }

    QPdfDocument               m_doc;
    int                        m_currentPage = 0;
    QString                    m_pageText;
    std::optional<QTextStream> m_stream;
};

class WordDocumentReader final : public DocumentReader {
public:
    explicit WordDocumentReader(const DocumentInfo &info)
        : DocumentReader(info)
        , m_doc(info.file.canonicalFilePath().toStdString())
    {
        m_doc.open();
        if (!m_doc.is_open())
            throw std::runtime_error(fmt::format("Failed to open DOCX: {}", info.file.canonicalFilePath()));

        m_paragraph = &m_doc.paragraphs();
        m_run       = &m_paragraph->runs();
        // TODO(jared): metadata for Word documents?
        postInit();
    }

protected:
    std::optional<QString> advance() override
    {
        // find non-space char
        qsizetype wordStart = 0;
        while (m_buffer.isEmpty() || m_buffer[wordStart].isSpace()) {
            if (m_buffer.isEmpty() && !fillBuffer())
                return std::nullopt;
            if (m_buffer[wordStart].isSpace() && ++wordStart >= m_buffer.size()) {
                m_buffer.clear();
                wordStart = 0;
            }
        }

        // find space char
        qsizetype wordEnd = wordStart + 1;
        while (wordEnd >= m_buffer.size() || !m_buffer[wordEnd].isSpace()) {
            if (wordEnd >= m_buffer.size() && !fillBuffer())
                break;
            if (!m_buffer[wordEnd].isSpace())
                ++wordEnd;
        }

        if (wordStart == wordEnd)
            return std::nullopt;

        auto size = wordEnd - wordStart;
        QString word = std::move(m_buffer);
        m_buffer = word.sliced(wordStart + size);
        if (wordStart == 0)
            word.resize(size);
        else
            word = word.sliced(wordStart, size);
        return word;
    }

    bool fillBuffer()
    {
        for (;;) {
            // get a run
            while (!m_run->has_next()) {
                // try next paragraph
                if (!m_paragraph->has_next())
                    return false;

                m_paragraph->next();
                m_buffer += u'\n';
            }

            bool foundText = false;
            auto &run = m_run->get_node();
            for (auto node = run.first_child(); node; node = node.next_sibling()) {
                std::string node_name = node.name();
                if (node_name == "w:t") {
                    const char *text = node.text().get();
                    if (*text) {
                        foundText = true;
                        m_buffer += QUtf8StringView(text);
                    }
                } else if (node_name == "w:br") {
                    m_buffer += u'\n';
                } else if (node_name == "w:tab") {
                    m_buffer += u'\t';
                }
            }

            m_run->next();
            if (foundText) return true;
        }
    }

    duckx::Document   m_doc;
    duckx::Paragraph *m_paragraph;
    duckx::Run       *m_run;
    QString           m_buffer;
};

class TxtDocumentReader final : public DocumentReader {
public:
    explicit TxtDocumentReader(const DocumentInfo &info)
        : DocumentReader(info)
        , m_file(info.file.canonicalFilePath())
    {
        if (!m_file.open(QIODevice::ReadOnly))
            throw std::runtime_error(fmt::format("Failed to open text file: {}", m_file.fileName()));

        m_stream.setDevice(&m_file);
        postInit();
    }

protected:
    std::optional<QString> advance() override
    {
        if (getError())
            return std::nullopt;
        while (!m_stream.atEnd()) {
            QString word;
            m_stream >> word;
            if (getError())
                return std::nullopt;
            if (!word.isEmpty())
                return word;
        }
        return std::nullopt;
    }

    std::optional<ChunkStreamer::Status> getError() const override
    {
        if (m_file.binarySeen())
            return ChunkStreamer::Status::BINARY_SEEN;
        if (m_file.error())
            return ChunkStreamer::Status::ERROR;
        return std::nullopt;
    }

    BinaryDetectingFile m_file;
    QTextStream m_stream;
};

} // namespace

std::unique_ptr<DocumentReader> DocumentReader::fromDocument(const DocumentInfo &doc)
{
    if (doc.isPdf())
        return std::make_unique<PdfDocumentReader>(doc);
    if (doc.isDocx())
        return std::make_unique<WordDocumentReader>(doc);
    return std::make_unique<TxtDocumentReader>(doc);
}

ChunkStreamer::ChunkStreamer(Database *database)
    : m_database(database) {}

ChunkStreamer::~ChunkStreamer() = default;

void ChunkStreamer::setDocument(const DocumentInfo &doc, int documentId, const QString &embeddingModel)
{
    auto docKey = doc.key();
    if (!m_docKey || *m_docKey != docKey) {
        m_docKey         = docKey;
        m_reader         = DocumentReader::fromDocument(doc);
        m_documentId     = documentId;
        m_embeddingModel = embeddingModel;
        m_chunk.clear();
        m_page = 0;

        // make sure the document doesn't already have any chunks
        if (m_database->m_documentIdCache.contains(documentId)) {
            QSqlQuery q(m_database->m_db);
            if (!m_database->removeChunksByDocumentId(q, documentId))
                handleDocumentError("ERROR: Cannot remove chunks of document", documentId, doc.file.canonicalPath(), q.lastError());
        }
    }
}

std::optional<DocumentInfo::key_type> ChunkStreamer::currentDocKey() const
{
    return m_docKey;
}

void ChunkStreamer::reset()
{
    m_docKey.reset();
}

ChunkStreamer::Status ChunkStreamer::step()
{
    // TODO: implement line_from/line_to
    constexpr int line_from = -1;
    constexpr int line_to = -1;
    const int folderId = m_reader->doc().folder;
    const int maxChunkSize = m_database->m_chunkSize;
    int nChunks = 0;
    int nAddedWords = 0;
    Status retval;

    for (;;) {
        if (auto error = m_reader->getError()) {
            m_docKey.reset(); // done processing
            retval = *error;
            break;
        }

        // get a word, if needed
        std::optional<QString> word = QString(); // empty string to disable EOF logic
        if (m_chunk.length() < maxChunkSize + 1) {
            word = m_reader->word();
            if (m_chunk.isEmpty())
                m_page = m_reader->page(); // page number of first word

            if (word) {
                m_chunk += *word;
                m_chunk += u' ';
                m_reader->nextWord();
                m_nChunkWords++;
            }
        }

        if (!word || m_chunk.length() >= maxChunkSize + 1) { // +1 for trailing space
            if (!m_chunk.isEmpty()) {
                int nThisChunkWords = 0;
                auto chunk = m_chunk; // copy

                // handle overlength chunks
                if (m_chunk.length() > maxChunkSize + 1) {
                    // find the final space
                    qsizetype chunkEnd = chunk.lastIndexOf(u' ', -2);

                    qsizetype spaceSize;
                    if (chunkEnd >= 0) {
                        // slice off the last word
                        spaceSize = 1;
                        Q_ASSERT(m_nChunkWords >= 1);
                        // one word left
                        nThisChunkWords = m_nChunkWords - 1;
                        m_nChunkWords = 1;
                    } else {
                        // slice the overlong word
                        spaceSize = 0;
                        chunkEnd = maxChunkSize;
                        // partial word left, don't count it
                        nThisChunkWords = m_nChunkWords;
                        m_nChunkWords = 0;
                    }
                    // save the second part, excluding space if any
                    m_chunk = chunk.sliced(chunkEnd + spaceSize);
                    // consume the first part
                    chunk.truncate(chunkEnd);
                } else {
                    nThisChunkWords = m_nChunkWords;
                    m_nChunkWords = 0;
                    // there is no second part
                    m_chunk.clear();
                    // consume the whole chunk, excluding space
                    chunk.chop(1);
                }
                Q_ASSERT(chunk.length() <= maxChunkSize);

                QSqlQuery q(m_database->m_db);
                int chunkId = 0;
                auto &metadata = m_reader->metadata();
                if (!m_database->addChunk(q,
                    m_documentId,
                    chunk,
                    m_reader->doc().file.fileName(), // basename
                    metadata.title,
                    metadata.author,
                    metadata.subject,
                    metadata.keywords,
                    m_page,
                    line_from,
                    line_to,
                    nThisChunkWords,
                    &chunkId
                )) {
                    qWarning() << "ERROR: Could not insert chunk into db" << q.lastError();
                }

                nAddedWords += nThisChunkWords;

                EmbeddingChunk toEmbed;
                toEmbed.model = m_embeddingModel;
                toEmbed.folder_id = folderId;
                toEmbed.chunk_id = chunkId;
                toEmbed.chunk = chunk;
                m_database->appendChunk(toEmbed);
                ++nChunks;
            }

            if (!word) {
                retval = Status::DOC_COMPLETE;
                m_docKey.reset(); // done processing
                break;
            }
        }

        if (m_database->scanQueueInterrupted()) {
            retval = Status::INTERRUPTED;
            break;
        }
    }

    if (nChunks) {
        CollectionItem item = m_database->guiCollectionItem(folderId);

        // Set the start update if we haven't done so already
        if (item.startUpdate <= item.lastUpdate && item.currentEmbeddingsToIndex == 0)
            m_database->setStartUpdateTime(item);

        item.currentEmbeddingsToIndex += nChunks;
        item.totalEmbeddingsToIndex += nChunks;
        item.totalWords += nAddedWords;
        m_database->updateGuiForCollectionItem(item);
    }

    return retval;
}

void Database::appendChunk(const EmbeddingChunk &chunk)
{
    m_chunkList.reserve(s_batchSize);
    m_chunkList.append(chunk);
    if (m_chunkList.size() >= s_batchSize)
        sendChunkList();
}

void Database::sendChunkList()
{
    m_embLLM->generateDocEmbeddingsAsync(m_chunkList);
    m_chunkList.clear();
}

void Database::handleEmbeddingsGenerated(const QVector<EmbeddingResult> &embeddings)
{
    Q_ASSERT(!embeddings.isEmpty());

    QList<Embedding> sqlEmbeddings;
    for (const auto &e: embeddings) {
        auto data = QByteArray::fromRawData(
            reinterpret_cast<const char *>(e.embedding.data()),
            e.embedding.size() * sizeof(e.embedding.front())
        );
        sqlEmbeddings.append({e.model, e.folder_id, e.chunk_id, std::move(data)});
    }

    transaction();

    QSqlQuery q(m_db);
    QHash<EmbeddingFolder, EmbeddingStat> stats;
    if (!sqlAddEmbeddings(q, sqlEmbeddings, stats)) {
        qWarning() << "Database ERROR: failed to add embeddings:" << q.lastError();
        return rollback();
    }

    commit();

    // FIXME(jared): embedding counts are per-collectionitem, not per-folder
    for (const auto &[key, stat]: std::as_const(stats).asKeyValueRange()) {
        if (!m_collectionMap.contains(key.folder_id)) continue;
        CollectionItem item = guiCollectionItem(key.folder_id);
        Q_ASSERT(item.currentEmbeddingsToIndex >= stat.nAdded + stat.nSkipped);
        if (item.currentEmbeddingsToIndex < stat.nAdded + stat.nSkipped) {
            qWarning() << "Database ERROR: underflow in current embeddings to index statistics";
            item.currentEmbeddingsToIndex = 0;
        } else {
            item.currentEmbeddingsToIndex -= stat.nAdded + stat.nSkipped;
        }

        Q_ASSERT(item.totalEmbeddingsToIndex >= stat.nSkipped);
        if (item.totalEmbeddingsToIndex < stat.nSkipped) {
            qWarning() << "Database ERROR: underflow in total embeddings to index statistics";
            item.totalEmbeddingsToIndex = 0;
        } else {
            item.totalEmbeddingsToIndex -= stat.nSkipped;
        }

        if (!stat.lastFile.isNull())
            item.fileCurrentlyProcessing = stat.lastFile;

        // Set the last update if we are done
        Q_ASSERT(item.startUpdate > item.lastUpdate);
        if (!item.indexing && item.currentEmbeddingsToIndex == 0)
            setLastUpdateTime(item);

        updateGuiForCollectionItem(item);
    }
}

void Database::handleErrorGenerated(const QVector<EmbeddingChunk> &chunks, const QString &error)
{
    /* FIXME(jared): errors are actually collection-specific because they are conditioned
     * on the embedding model, but this sets the error on all collections for a given
     * folder */

    QSet<int> folder_ids;
    for (const auto &c: chunks) { folder_ids << c.folder_id; }

    for (int fid: folder_ids) {
        if (!m_collectionMap.contains(fid)) continue;
        CollectionItem item = guiCollectionItem(fid);
        item.error = error;
        updateGuiForCollectionItem(item);
    }
}

size_t Database::countOfDocuments(int folder_id) const
{
    if (auto it = m_docsToScan.find(folder_id); it != m_docsToScan.end())
        return it->second.size();
    return 0;
}

size_t Database::countOfBytes(int folder_id) const
{
    if (auto it = m_docsToScan.find(folder_id); it != m_docsToScan.end()) {
        size_t totalBytes = 0;
        for (const DocumentInfo &f : it->second)
            totalBytes += f.file.size();
        return totalBytes;
    }
    return 0;
}

DocumentInfo Database::dequeueDocument()
{
    Q_ASSERT(!m_docsToScan.empty());
    auto firstEntry = m_docsToScan.begin();
    auto &[firstKey, queue] = *firstEntry;
    Q_ASSERT(!queue.empty());
    DocumentInfo result = std::move(queue.front());
    queue.pop_front();
    if (queue.empty())
        m_docsToScan.erase(firstEntry);
    return result;
}

void Database::removeFolderFromDocumentQueue(int folder_id)
{
    if (auto queueIt = m_docsToScan.find(folder_id); queueIt != m_docsToScan.end()) {
        if (auto key = m_chunkStreamer.currentDocKey()) {
            if (ranges::any_of(queueIt->second, [&key](auto &d) { return d.key() == key; }))
                m_chunkStreamer.reset(); // done with this document
        }
        // remove folder from queue
        m_docsToScan.erase(queueIt);
    }
}

void Database::enqueueDocumentInternal(DocumentInfo &&info, bool prepend)
{
    auto &queue = m_docsToScan[info.folder];
    queue.insert(prepend ? queue.begin() : queue.end(), std::move(info));
}

void Database::enqueueDocuments(int folder_id, std::list<DocumentInfo> &&infos)
{
    // enqueue all documents
    auto &queue = m_docsToScan[folder_id];
    queue.splice(queue.end(), std::move(infos));

    CollectionItem item = guiCollectionItem(folder_id);
    item.currentDocsToIndex = queue.size();
    item.totalDocsToIndex = queue.size();
    const size_t bytes = countOfBytes(folder_id);
    item.currentBytesToIndex = bytes;
    item.totalBytesToIndex = bytes;
    updateGuiForCollectionItem(item);
    m_scanIntervalTimer->start();
}

bool Database::scanQueueInterrupted() const
{
    return m_scanDurationTimer.elapsed() >= 100;
}

void Database::scanQueueBatch()
{
    transaction();

    m_scanDurationTimer.start();

    // scan for up to the maximum scan duration or until we run out of documents
    while (!m_docsToScan.empty()) {
        scanQueue();
        if (scanQueueInterrupted())
            break;
    }

    commit();

    if (m_docsToScan.empty())
        m_scanIntervalTimer->stop();
}

void Database::scanQueue()
{
    DocumentInfo info = dequeueDocument();
    const size_t countForFolder = countOfDocuments(info.folder);
    const int folder_id = info.folder;

    // Update info
    info.file.stat();

    // If the doc has since been deleted or no longer readable, then we schedule more work and return
    // leaving the cleanup for the cleanup handler
    if (!info.file.exists() || !info.file.isReadable())
        return updateFolderToIndex(folder_id, countForFolder);

    const qint64 document_time = info.file.fileTime(QFile::FileModificationTime).toMSecsSinceEpoch();
    const QString document_path = info.file.canonicalFilePath();
    const bool currentlyProcessing = info.currentlyProcessing;

    // Check and see if we already have this document
    QSqlQuery q(m_db);
    int existing_id = -1;
    qint64 existing_time = -1;
    if (!selectDocument(q, document_path, &existing_id, &existing_time)) {
        handleDocumentError("ERROR: Cannot select document",
            existing_id, document_path, q.lastError());
        return updateFolderToIndex(folder_id, countForFolder);
    }

    // If we have the document, we need to compare the last modification time and if it is newer
    // we must rescan the document, otherwise return
    if (existing_id != -1 && !currentlyProcessing) {
        Q_ASSERT(existing_time != -1);
        if (document_time == existing_time) {
            // No need to rescan, but we do have to schedule next
            return updateFolderToIndex(folder_id, countForFolder);
        }
        if (!removeChunksByDocumentId(q, existing_id)) {
            handleDocumentError("ERROR: Cannot remove chunks of document",
                existing_id, document_path, q.lastError());
            return updateFolderToIndex(folder_id, countForFolder);
        }
        updateCollectionStatistics();
    }

    // Update the document_time for an existing document, or add it for the first time now
    int document_id = existing_id;
    if (!currentlyProcessing) {
        if (document_id != -1) {
            if (!updateDocument(q, document_id, document_time)) {
                handleDocumentError("ERROR: Could not update document_time",
                    document_id, document_path, q.lastError());
                return updateFolderToIndex(folder_id, countForFolder);
            }
        } else {
            if (!addDocument(q, folder_id, document_time, document_path, &document_id)) {
                handleDocumentError("ERROR: Could not add document",
                    document_id, document_path, q.lastError());
                return updateFolderToIndex(folder_id, countForFolder);
            }

            CollectionItem item = guiCollectionItem(folder_id);
            item.totalDocs += 1;
            updateGuiForCollectionItem(item);
        }
    }

    // Get the embedding model for this folder
    // FIXME(jared): there can be more than one since we allow one folder to be in multiple collections
    QString embedding_model;
    if (!sqlGetFolderEmbeddingModel(q, folder_id, embedding_model)) {
        handleDocumentError("ERROR: Could not get embedding model",
            document_id, document_path, q.lastError());
        return updateFolderToIndex(folder_id, countForFolder);
    }

    Q_ASSERT(document_id != -1);

    {
        try {
            m_chunkStreamer.setDocument(info, document_id, embedding_model);
        } catch (const std::runtime_error &e) {
            qWarning() << "LocalDocs ERROR:" << e.what();
            goto dequeue;
        }
    }

    switch (m_chunkStreamer.step()) {
    case ChunkStreamer::Status::INTERRUPTED:
        info.currentlyProcessing = true;
        enqueueDocumentInternal(std::move(info), /*prepend*/ true);
        return updateFolderToIndex(folder_id, countForFolder + 1);
    case ChunkStreamer::Status::BINARY_SEEN:
        /* When we see a binary file, we treat it like an empty file so we know not to
         * scan it again. All existing chunks are removed, and in-progress embeddings
         * are ignored when they complete. */
        qInfo() << "LocalDocs: Ignoring file with binary data:" << document_path;

        // this will also ensure in-flight embeddings are ignored
        if (!removeChunksByDocumentId(q, existing_id))
            handleDocumentError("ERROR: Cannot remove chunks of document", existing_id, document_path, q.lastError());
        updateCollectionStatistics();
        break;
    case ChunkStreamer::Status::ERROR:
        qWarning() << "error reading" << document_path;
        break;
    case ChunkStreamer::Status::DOC_COMPLETE:
        ;
    }

dequeue:
    auto item = guiCollectionItem(folder_id);
    Q_ASSERT(item.currentBytesToIndex >= info.file.size());
    if (item.currentBytesToIndex < info.file.size()) {
        qWarning() << "Database ERROR: underflow in current bytes to index statistics";
        item.currentBytesToIndex = 0;
    } else {
        item.currentBytesToIndex -= info.file.size();
    }
    updateGuiForCollectionItem(item);
    return updateFolderToIndex(folder_id, countForFolder);
}

void Database::scanDocuments(int folder_id, const QString &folder_path)
{
#if defined(DEBUG)
    qDebug() << "scanning folder for documents" << folder_path;
#endif

    QDirIterator it(folder_path, QDir::Readable | QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    std::list<DocumentInfo> infos;
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        if (fileInfo.isDir()) {
            addFolderToWatch(fileInfo.canonicalFilePath());
            continue;
        }

        if (!m_scannedFileExtensions.contains(fileInfo.suffix(), Qt::CaseInsensitive))
            continue;

        infos.push_back({ folder_id, fileInfo });
    }

    if (!infos.empty()) {
        CollectionItem item = guiCollectionItem(folder_id);
        item.indexing = true;
        updateGuiForCollectionItem(item);
        enqueueDocuments(folder_id, std::move(infos));
    } else {
        updateFolderToIndex(folder_id, 0, false);
    }
}

void Database::start()
{
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &Database::directoryChanged);
    connect(m_embLLM, &EmbeddingLLM::embeddingsGenerated, this, &Database::handleEmbeddingsGenerated);
    connect(m_embLLM, &EmbeddingLLM::errorGenerated, this, &Database::handleErrorGenerated);
    m_scanIntervalTimer->callOnTimeout(this, &Database::scanQueueBatch);

    const QString modelPath = MySettings::globalInstance()->modelPath();
    QList<CollectionItem> oldCollections;

    if (!openLatestDb(modelPath, oldCollections)) {
        m_databaseValid = false;
    } else if (!initDb(modelPath, oldCollections)) {
        m_databaseValid = false;
    } else {
        cleanDB();
        ftsIntegrityCheck();
        QSqlQuery q(m_db);
        if (!refreshDocumentIdCache(q)) {
            m_databaseValid = false;
        } else {
            addCurrentFolders();
        }
    }

    if (!m_databaseValid)
        emit databaseValidChanged();
}

void Database::addCurrentFolders()
{
#if defined(DEBUG)
    qDebug() << "addCurrentFolders";
#endif

    QSqlQuery q(m_db);
    QList<CollectionItem> collections;
    if (!selectAllFromCollections(q, &collections)) {
        qWarning() << "ERROR: Cannot select collections" << q.lastError();
        return;
    }

    guiCollectionListUpdated(collections);

    scheduleUncompletedEmbeddings();

    for (const auto &i : collections) {
        if (!i.forceIndexing) {
            addFolderToWatch(i.folder_path);
            scanDocuments(i.folder_id, i.folder_path);
        }
    }

    updateCollectionStatistics();
}

void Database::scheduleUncompletedEmbeddings()
{
    QHash<IncompleteChunk, QStringList> chunkList;
    QSqlQuery q(m_db);
    if (!selectAllUncompletedChunks(q, chunkList)) {
        qWarning() << "ERROR: Cannot select uncompleted chunks" << q.lastError();
        return;
    }

    if (chunkList.isEmpty())
        return;

    // map of folder_id -> chunk count
    QMap<int, int> folderNChunks;
    for (auto it = chunkList.keyBegin(), end = chunkList.keyEnd(); it != end; ++it) {
        int folder_id = it->folder_id;

        if (folderNChunks.contains(folder_id)) continue;
        int total = 0;
        if (!selectCountChunks(q, folder_id, total)) {
            qWarning() << "ERROR: Cannot count total chunks" << q.lastError();
            return;
        }
        folderNChunks.insert(folder_id, total);
    }

    // map of (folder_id, collection) -> incomplete count
    QMap<QPair<int, QString>, int> itemNIncomplete;
    for (const auto &[chunk, collections]: std::as_const(chunkList).asKeyValueRange())
        for (const auto &collection: std::as_const(collections))
            itemNIncomplete[{ chunk.folder_id, collection }]++;

    for (const auto &[key, nIncomplete]: std::as_const(itemNIncomplete).asKeyValueRange()) {
        const auto &[folder_id, collection] = key;

        /* FIXME(jared): this needs to be split by collection because different
         * collections have different embedding models */
        int total = folderNChunks.value(folder_id);
        CollectionItem item = guiCollectionItem(folder_id);
        item.totalEmbeddingsToIndex = total;
        item.currentEmbeddingsToIndex = nIncomplete;
        updateGuiForCollectionItem(item);
    }

    for (auto it = chunkList.keyBegin(), end = chunkList.keyEnd(); it != end;) {
        QList<EmbeddingChunk> batch;
        for (; it != end && batch.size() < s_batchSize; ++it)
            batch.append({ /*model*/ it->embedding_model, /*folder_id*/ it->folder_id, /*chunk_id*/ it->chunk_id, /*chunk*/ it->text });
        Q_ASSERT(!batch.isEmpty());
        m_embLLM->generateDocEmbeddingsAsync(batch);
    }
}

void Database::updateCollectionStatistics()
{
    QSqlQuery q(m_db);
    QList<CollectionItem> collections;
    if (!selectAllFromCollections(q, &collections)) {
        qWarning() << "ERROR: Cannot select collections" << q.lastError();
        return;
    }

    for (const auto &i: std::as_const(collections)) {
        int total_docs = 0;
        int total_words = 0;
        int total_tokens = 0;
        if (!selectCountStatistics(q, i.folder_id, &total_docs, &total_words, &total_tokens)) {
            qWarning() << "ERROR: could not count statistics for folder" << q.lastError();
        } else {
            CollectionItem item = guiCollectionItem(i.folder_id);
            item.totalDocs = total_docs;
            item.totalWords = total_words;
            item.totalTokens = total_tokens;
            updateGuiForCollectionItem(item);
        }
    }
}

int Database::checkAndAddFolderToDB(const QString &path)
{
    QFileInfo info(path);
    if (!info.exists() || !info.isReadable()) {
        qWarning() << "ERROR: Cannot add folder that doesn't exist or not readable" << path;
        return -1;
    }

    QSqlQuery q(m_db);
    int folder_id = -1;

    // See if the folder exists in the db
    if (!selectFolder(q, path, &folder_id)) {
        qWarning() << "ERROR: Cannot select folder from path" << path << q.lastError();
        return -1;
    }

    // Add the folder
    if (folder_id == -1 && !addFolderToDB(q, path, &folder_id)) {
        qWarning() << "ERROR: Cannot add folder to db with path" << path << q.lastError();
        return -1;
    }

    Q_ASSERT(folder_id != -1);
    return folder_id;
}

void Database::forceIndexing(const QString &collection, const QString &embedding_model)
{
    Q_ASSERT(!embedding_model.isNull());

    QSqlQuery q(m_db);
    QList<QPair<int, QString>> folders;
    if (!selectFoldersFromCollection(q, collection, &folders)) {
        qWarning() << "ERROR: Cannot select folders from collections" << collection << q.lastError();
        return;
    }

    if (!setCollectionEmbeddingModel(q, collection, embedding_model)) {
        qWarning().nospace() << "ERROR: Cannot set embedding model for collection " << collection << ": "
                             << q.lastError();
        return;
    }

    for (const auto &folder: std::as_const(folders)) {
        CollectionItem item = guiCollectionItem(folder.first);
        item.embeddingModel = embedding_model;
        item.forceIndexing = false;
        updateGuiForCollectionItem(item);
        addFolderToWatch(folder.second);
        scanDocuments(folder.first, folder.second);
    }
}

void Database::forceRebuildFolder(const QString &path)
{
    QSqlQuery q(m_db);
    int folder_id;
    if (!selectFolder(q, path, &folder_id)) {
        qWarning().nospace() << "Database ERROR: Cannot select folder from path " << path << ": " << q.lastError();
        return;
    }

    Q_ASSERT(!m_docsToScan.contains(folder_id));

    transaction();

    if (!sqlRemoveDocsByFolderPath(q, path)) {
        qWarning().nospace() << "Database ERROR: Cannot remove chunks for folder " << path << ": " << q.lastError();
        return rollback();
    }

    commit();

    updateCollectionStatistics();

    // We now have zero embeddings. Document progress will be updated by scanDocuments.
    // FIXME(jared): this updates the folder, but these values should also depend on the collection
    CollectionItem item = guiCollectionItem(folder_id);
    item.currentEmbeddingsToIndex = item.totalEmbeddingsToIndex = 0;
    updateGuiForCollectionItem(item);

    scanDocuments(folder_id, path);
}

bool Database::addFolder(const QString &collection, const QString &path, const QString &embedding_model)
{
    // add the folder, if needed
    const int folder_id = checkAndAddFolderToDB(path);
    if (folder_id == -1)
        return false;

    std::optional<CollectionItem> item;
    QSqlQuery q(m_db);
    if (!selectCollectionByName(q, collection, item)) {
        qWarning().nospace() << "Database ERROR: Cannot select collection " << collection << ": " << q.lastError();
        return false;
    }

    // add the collection, if needed
    if (!item) {
        item.emplace();
        if (!addCollection(q, collection, QDateTime() /*start_update*/, QDateTime() /*last_update*/,
            embedding_model /*embedding_model*/, *item)) {
            qWarning().nospace() << "ERROR: Cannot add collection " << collection << ": " << q.lastError();
            return false;
        }
    }

    // link the folder and the collection, if needed
    int res = addCollectionItem(q, item->collection_id, folder_id);
    if (res < 0) { // error
        qWarning().nospace() << "Database ERROR: Cannot add folder " << path << " to collection " << collection << ": "
                             << q.lastError();
        return false;
    }

    // add the new collection item to the UI
    if (res == 1) { // new item added
        item->folder_path = path;
        item->folder_id = folder_id;
        addGuiCollectionItem(item.value());

        // note: this is the existing embedding model if the collection was found
        if (!item->embeddingModel.isNull()) {
            addFolderToWatch(path);
            scanDocuments(folder_id, path);
        }
    }
    return true;
}

void Database::removeFolder(const QString &collection, const QString &path)
{
#if defined(DEBUG)
    qDebug() << "removeFolder" << path;
#endif

    QSqlQuery q(m_db);
    int folder_id = -1;

    // See if the folder exists in the db
    if (!selectFolder(q, path, &folder_id)) {
        qWarning() << "ERROR: Cannot select folder from path" << path << q.lastError();
        return;
    }

    // If we don't have a folder_id in the db, then something bad has happened
    Q_ASSERT(folder_id != -1);
    if (folder_id == -1) {
        qWarning() << "ERROR: Collected folder does not exist in db" << path;
        m_watcher->removePath(path);
        return;
    }

    transaction();

    if (removeFolderInternal(collection, folder_id, path)) {
        commit();
    } else {
        rollback();
    }
}

bool Database::removeFolderInternal(const QString &collection, int folder_id, const QString &path)
{
    // Remove it from the collection
    QSqlQuery q(m_db);
    int nRemaining = removeCollectionFolder(q, collection, folder_id);
    if (nRemaining == -1) {
        qWarning().nospace() << "Database ERROR: Cannot remove collection " << collection << " from folder "
                             << folder_id << ": " << q.lastError();
        return false;
    }
    removeGuiFolderById(collection, folder_id);

    if (!sqlPruneCollections(q)) {
        qWarning() << "Database ERROR: Cannot prune collections:" << q.lastError();
        return false;
    }

    // Keep folder if it is still referenced
    if (nRemaining)
        return true;

    // Remove the last reference to a folder

    // First remove all upcoming jobs associated with this folder
    removeFolderFromDocumentQueue(folder_id);

    // Get a list of all documents associated with folder
    QList<int> documentIds;
    if (!selectDocuments(q, folder_id, &documentIds)) {
        qWarning() << "ERROR: Cannot select documents" << folder_id << q.lastError();
        return false;
    }

    // Remove all chunks and documents associated with this folder
    for (int document_id: std::as_const(documentIds)) {
        if (!removeChunksByDocumentId(q, document_id)) {
            qWarning() << "ERROR: Cannot remove chunks of document_id" << document_id << q.lastError();
            return false;
        }

        if (!removeDocument(q, document_id)) {
            qWarning() << "ERROR: Cannot remove document_id" << document_id << q.lastError();
            return false;
        }
    }

    if (!removeFolderFromDB(q, folder_id)) {
        qWarning() << "ERROR: Cannot remove folder_id" << folder_id << q.lastError();
        return false;
    }

    m_collectionMap.remove(folder_id);
    removeFolderFromWatch(path);
    return true;
}

void Database::addFolderToWatch(const QString &path)
{
#if defined(DEBUG)
    qDebug() << "addFolderToWatch" << path;
#endif
    // pre-check because addPath returns false for already watched paths
    if (!m_watchedPaths.contains(path)) {
        if (!m_watcher->addPath(path))
            qWarning() << "Database::addFolderToWatch: failed to watch" << path;
        // add unconditionally to suppress repeated warnings
        m_watchedPaths << path;
    }
}

void Database::removeFolderFromWatch(const QString &path)
{
#if defined(DEBUG)
    qDebug() << "removeFolderFromWatch" << path;
#endif
    QDirIterator it(path, QDir::Readable | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QStringList children { path };
    while (it.hasNext())
        children.append(it.next());

    m_watcher->removePaths(children);
    m_watchedPaths -= QSet(children.begin(), children.end());
}

QList<int> Database::searchEmbeddingsHelper(const std::vector<float> &query, QSqlQuery &q, int nNeighbors)
{
    constexpr int BATCH_SIZE = 2048;

    const int n_embd = query.size();
    const us::metric_punned_t metric(n_embd, us::metric_kind_t::ip_k); // inner product

    us::executor_default_t executor(std::thread::hardware_concurrency());
    us::exact_search_t search;

    QList<int> batchChunkIds;
    QList<float> batchEmbeddings;
    batchChunkIds.reserve(BATCH_SIZE);
    batchEmbeddings.reserve(BATCH_SIZE * n_embd);

    struct Result { int chunkId; us::distance_punned_t dist; };
    QList<Result> results;

    // The q parameter is expected to be the result of a QSqlQuery returning (chunk_id, embedding) pairs
    while (q.at() != QSql::AfterLastRow) { // batches
        batchChunkIds.clear();
        batchEmbeddings.clear();

        while (batchChunkIds.count() < BATCH_SIZE && q.next()) { // batch
            batchChunkIds << q.value(0).toInt();
            batchEmbeddings.resize(batchEmbeddings.size() + n_embd);
            QVariant embdCol = q.value(1);
            if (embdCol.userType() != QMetaType::QByteArray) {
                qWarning() << "Database ERROR: Expected embedding to be blob, got" << embdCol.userType();
                return {};
            }
            auto *embd = static_cast<const QByteArray *>(embdCol.constData());
            const int embd_stride = n_embd * sizeof(float);
            if (embd->size() != embd_stride) {
                qWarning() << "Database ERROR: Expected embedding to be" << embd_stride << "bytes, got"
                           << embd->size();
                return {};
            }
            memcpy(&*(batchEmbeddings.end() - n_embd), embd->constData(), embd_stride);
        }

        int nBatch = batchChunkIds.count();
        if (!nBatch)
            break;

        // get top-k nearest neighbors of this batch
        int kBatch = qMin(nNeighbors, nBatch);
        us::exact_search_results_t batchResults = search(
            (us::byte_t const *)batchEmbeddings.data(), nBatch, n_embd * sizeof(float),
            (us::byte_t const *)query.data(),           1,      n_embd * sizeof(float),
            kBatch, metric
        );

        for (int i = 0; i < kBatch; ++i) {
            auto offset = batchResults.at(0)[i].offset;
            us::distance_punned_t distance = batchResults.at(0)[i].distance;
            results.append({batchChunkIds[offset], distance});
        }
    }

    // get top-k nearest neighbors of combined results
    nNeighbors = qMin(nNeighbors, results.size());
    std::partial_sort(
        results.begin(), results.begin() + nNeighbors, results.end(),
        [](const Result &a, const Result &b) { return a.dist < b.dist; }
    );

    QList<int> chunkIds;
    chunkIds.reserve(nNeighbors);
    for (int i = 0; i < nNeighbors; i++)
        chunkIds << results[i].chunkId;
    return chunkIds;
}

QList<int> Database::searchEmbeddings(const std::vector<float> &query, const QList<QString> &collections,
    int nNeighbors)
{
    QSqlQuery q(m_db);
    if (!q.exec(GET_COLLECTION_EMBEDDINGS_SQL.arg(collections.join("', '")))) {
        qWarning() << "Database ERROR: Failed to exec embeddings query:" << q.lastError();
        return {};
    }
    return searchEmbeddingsHelper(query, q, nNeighbors);
}

QList<int> Database::scoreChunks(const std::vector<float> &query, const QList<int> &chunks)
{
    QList<QString> chunkStrings;
    for (int id : chunks)
        chunkStrings << QString::number(id);
    QSqlQuery q(m_db);
    if (!q.exec(GET_CHUNK_EMBEDDINGS_SQL.arg(chunkStrings.join(", ")))) {
        qWarning() << "Database ERROR: Failed to exec embeddings query:" << q.lastError();
        return {};
    }
    return searchEmbeddingsHelper(query, q, chunks.size());
}

QList<Database::BM25Query> Database::queriesForFTS5(const QString &input)
{
    // Escape double quotes by adding a second double quote
    QString escapedInput = input;
    escapedInput.replace("\"", "\"\"");

    static QRegularExpression spaces("\\s+");
    QStringList oWords = escapedInput.split(spaces, Qt::SkipEmptyParts);

    QList<BM25Query> queries;

    // Start by trying to match the entire input
    BM25Query e;
    e.isExact = true;
    e.input = oWords.join(" ");
    e.query = "\"" + oWords.join(" ") + "\"";
    e.qlength = oWords.size();
    e.ilength = oWords.size();
    queries << e;

    // https://github.com/igorbrigadir/stopwords?tab=readme-ov-file
    // Lucene, Solr, Elastisearch
    static const QSet<QString> stopWords = {
        "a", "an", "and", "are", "as", "at", "be", "but", "by",
        "for", "if", "in", "into", "is", "it", "no", "not", "of",
        "on", "or", "such", "that", "the", "their", "then", "there",
        "these", "they", "this", "to", "was", "will", "with"
    };

    QStringList quotedWords;
    for (const QString &w : oWords)
        if (!stopWords.contains(w.toLower()))
            quotedWords << "\"" + w + "\"";

    BM25Query b;
    b.input = oWords.join(" ");
    b.query = "(" + quotedWords.join(" OR ") + ")";
    b.qlength = 1; // length of phrase
    b.ilength = oWords.size();
    b.rlength = oWords.size() - quotedWords.size();
    queries << b;
    return queries;
}

QList<int> Database::searchBM25(const QString &query, const QList<QString> &collections, BM25Query &bm25q, int k)
{
    struct SearchResult { int chunkId; float score; };
    QList<BM25Query> bm25Queries = queriesForFTS5(query);

    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare(SELECT_CHUNKS_FTS_SQL.arg(collections.join("', '"), QString::number(k)));

    QList<SearchResult> results;
    for (auto &bm25Query : std::as_const(bm25Queries)) {
        sqlQuery.addBindValue(bm25Query.query);

        if (!sqlQuery.exec()) {
            qWarning() << "Database ERROR: Failed to execute BM25 query:" << sqlQuery.lastError();
            return {};
        }

        if (sqlQuery.next()) {
            // Save the query that was used to produce results
            bm25q = bm25Query;
            break;
        }
    }

    if (sqlQuery.at() != QSql::AfterLastRow) {
        do {
            const int chunkId = sqlQuery.value(0).toInt();
            const float score = sqlQuery.value(1).toFloat();
            results.append({chunkId, score});
        } while (sqlQuery.next());
    }

    k = qMin(k, results.size());
    std::partial_sort(
        results.begin(), results.begin() + k, results.end(),
        [](const SearchResult &a, const SearchResult &b) { return a.score < b.score; }
    );

    QList<int> chunkIds;
    chunkIds.reserve(k);
    for (int i = 0; i < k; i++)
        chunkIds << results[i].chunkId;
    return chunkIds;
}

float Database::computeBM25Weight(const Database::BM25Query &bm25q)
{
    float bmWeight = 0.0f;
    if (bm25q.isExact) {
        bmWeight = 0.9f; // the highest we give
    } else {
        // qlength is the length of the phrases in the query by number of distinct words
        // ilength is the length of the natural language query by number of distinct words
        // rlength is the number of stop words removed from the natural language query to form the query

        // calculate the query length weight based on the ratio of query terms to meaningful terms.
        // this formula adjusts the weight with the empirically determined insight that BM25's
        // effectiveness decreases as query length increases.
        float queryLengthWeight = 1 / powf(float(bm25q.ilength - bm25q.rlength), 2);
        queryLengthWeight = qBound(0.0f, queryLengthWeight, 1.0f);

        // the weighting is bound between 1/4 and 3/4 which was determined empirically to work well
        // with the beir nfcorpus, scifact, fiqa and trec-covid datasets along with our embedding
        // model
        bmWeight = 0.25f + queryLengthWeight * 0.50f;
    }

#if 0
    qDebug()
        << "bm25q.type"         << bm25q.type
        << "bm25q.qlength"      << bm25q.qlength
        << "bm25q.ilength"      << bm25q.ilength
        << "bm25q.rlength"      << bm25q.rlength
        << "bmWeight"           << bmWeight;
#endif

    return bmWeight;
}

QList<int> Database::reciprocalRankFusion(const std::vector<float> &query, const QList<int> &embeddingResults,
    const QList<int> &bm25Results, const BM25Query &bm25q, int k)
{
    // We default to the embedding results and augment with bm25 if any
    QList<int> results = embeddingResults;

    QList<int> missingScores;
    QHash<int, int> bm25Ranks;
    for (int i = 0; i < bm25Results.size(); ++i) {
        if (!results.contains(bm25Results[i]))
            missingScores.append(bm25Results[i]);
        bm25Ranks[bm25Results[i]] = i + 1;
    }

    if (!missingScores.isEmpty()) {
        QList<int> scored = scoreChunks(query, missingScores);
        results << scored;
    }

    QHash<int, int> embeddingRanks;
    for (int i = 0; i < results.size(); ++i)
        embeddingRanks[results[i]] = i + 1;

    const float bmWeight = bm25Results.isEmpty() ? 0 : computeBM25Weight(bm25q);

    // From the paper: "Reciprocal Rank Fusion outperforms Condorcet and individual Rank Learning Methods"
    // doi: 10.1145/1571941.157211
    const int fusion_k = 60;

    std::stable_sort(
        results.begin(), results.end(),
        [&](const int &a, const int &b) {
            // Reciprocal Rank Fusion (RRF)
            const int aBm25Rank = bm25Ranks.value(a, bm25Results.size() + 1);
            const int aEmbeddingRank = embeddingRanks.value(a, embeddingResults.size() + 1);
            Q_ASSERT(embeddingRanks.contains(a));

            const int bBm25Rank = bm25Ranks.value(b, bm25Results.size() + 1);
            const int bEmbeddingRank = embeddingRanks.value(b, embeddingResults.size() + 1);
            Q_ASSERT(embeddingRanks.contains(b));

            const float aBm25Score = 1.0f / (fusion_k + aBm25Rank);
            const float bBm25Score = 1.0f / (fusion_k + bBm25Rank);
            const float aEmbeddingScore = 1.0f / (fusion_k + aEmbeddingRank);
            const float bEmbeddingScore = 1.0f / (fusion_k + bEmbeddingRank);
            const float aWeightedScore = bmWeight * aBm25Score + (1.f - bmWeight) * aEmbeddingScore;
            const float bWeightedScore = bmWeight * bBm25Score + (1.f - bmWeight) * bEmbeddingScore;

            // Higher RRF score means better ranking, so we use greater than for sorting
            return aWeightedScore > bWeightedScore;
        }
    );

    k = qMin(k, results.size());
    results.resize(k);
    return results;
}

QList<int> Database::searchDatabase(const QString &query, const QList<QString> &collections, int k)
{
    std::vector<float> queryEmbd = m_embLLM->generateQueryEmbedding(query);
    if (queryEmbd.empty()) {
        qDebug() << "ERROR: generating embeddings returned a null result";
        return { };
    }

    const QList<int> embeddingResults = searchEmbeddings(queryEmbd, collections, k);
    BM25Query bm25q;
    const QList<int> bm25Results = searchBM25(query, collections, bm25q, k);
    return reciprocalRankFusion(queryEmbd, embeddingResults, bm25Results, bm25q, k);
}

void Database::retrieveFromDB(const QList<QString> &collections, const QString &text, int retrievalSize,
    QList<ResultInfo> *results)
{
#if defined(DEBUG)
    qDebug() << "retrieveFromDB" << collections << text << retrievalSize;
#endif

    QList<int> searchResults = searchDatabase(text, collections, retrievalSize);
    if (searchResults.isEmpty())
        return;

    QSqlQuery q(m_db);
    if (!selectChunk(q, searchResults)) {
        qDebug() << "ERROR: selecting chunks:" << q.lastError();
        return;
    }

    QHash<int, ResultInfo> tempResults;
    while (q.next()) {
        const int rowid = q.value(0).toInt();
        const QString document_path = q.value(2).toString();
        const QString chunk_text = q.value(3).toString();
        const QString date = QDateTime::fromMSecsSinceEpoch(q.value(1).toLongLong()).toString("yyyy, MMMM dd");
        const QString file = q.value(4).toString();
        const QString title = q.value(5).toString();
        const QString author = q.value(6).toString();
        const int page = q.value(7).toInt();
        const int from = q.value(8).toInt();
        const int to = q.value(9).toInt();
        const QString collectionName = q.value(10).toString();
        ResultInfo info;
        info.collection = collectionName;
        info.path = document_path;
        info.file = file;
        info.title = title;
        info.author = author;
        info.date = date;
        info.text = chunk_text;
        info.page = page;
        info.from = from;
        info.to = to;
        tempResults.insert(rowid, info);
#if defined(DEBUG)
        qDebug() << "retrieve rowid:" << rowid
                 << "chunk_text:" << chunk_text;
#endif
    }

    for (int id : searchResults)
        if (tempResults.contains(id))
            results->append(tempResults.value(id));
}

bool Database::ftsIntegrityCheck()
{
    QSqlQuery q(m_db);

    // Returns an error executing sql if it the integrity check fails
    // See: https://www.sqlite.org/fts5.html#the_integrity_check_command
    const bool success = q.exec(FTS_INTEGRITY_SQL);
    if (!success && q.lastError().nativeErrorCode() != "267" /*SQLITE_CORRUPT_VTAB from sqlite header*/) {
        qWarning() << "ERROR: Cannot prepare sql for fts integrity check" << q.lastError();
        return false;
    }

    if (!success && !q.exec(FTS_REBUILD_SQL)) {
        qWarning() << "ERROR: Cannot exec sql for fts rebuild" << q.lastError();
        return false;
    }

    return true;
}

// FIXME This is very slow and non-interruptible and when we close the application and we're
// cleaning a large table this can cause the app to take forever to shut down. This would ideally be
// interruptible and we'd continue 'cleaning' when we restart
bool Database::cleanDB()
{
#if defined(DEBUG)
    qDebug() << "cleanDB";
#endif

    // Scan all folders in db to make sure they still exist
    QSqlQuery q(m_db);
    QList<CollectionItem> collections;
    if (!selectAllFromCollections(q, &collections)) {
        qWarning() << "ERROR: Cannot select collections" << q.lastError();
        return false;
    }

    transaction();

    for (const auto &i: std::as_const(collections)) {
        // Find the path for the folder
        QFileInfo info(i.folder_path);
        if (!info.exists() || !info.isReadable()) {
#if defined(DEBUG)
            qDebug() << "clean db removing folder" << i.folder_id << i.folder_path;
#endif
            if (!removeFolderInternal(i.collection, i.folder_id, i.folder_path)) {
                rollback();
                return false;
            }
        }
    }

    // Scan all documents in db to make sure they still exist
    if (!q.prepare(SELECT_ALL_DOCUMENTS_SQL)) {
        qWarning() << "ERROR: Cannot prepare sql for select all documents" << q.lastError();
        rollback();
        return false;
    }

    if (!q.exec()) {
        qWarning() << "ERROR: Cannot exec sql for select all documents" << q.lastError();
        rollback();
        return false;
    }

    while (q.next()) {
        int document_id = q.value(0).toInt();
        QString document_path = q.value(1).toString();
        QFileInfo info(document_path);
        if (info.exists() && info.isReadable() && m_scannedFileExtensions.contains(info.suffix(), Qt::CaseInsensitive))
            continue;

#if defined(DEBUG)
        qDebug() << "clean db removing document" << document_id << document_path;
#endif

        // Remove all chunks and documents that either don't exist or have become unreadable
        QSqlQuery query(m_db);
        if (!removeChunksByDocumentId(query, document_id)) {
            qWarning() << "ERROR: Cannot remove chunks of document_id" << document_id << query.lastError();
            rollback();
            return false;
        }

        if (!removeDocument(query, document_id)) {
            qWarning() << "ERROR: Cannot remove document_id" << document_id << query.lastError();
            rollback();
            return false;
        }
    }

    commit();
    return true;
}

void Database::changeChunkSize(int chunkSize)
{
    if (chunkSize == m_chunkSize)
        return;

#if defined(DEBUG)
    qDebug() << "changeChunkSize" << chunkSize;
#endif

    QSqlQuery q(m_db);
    // Scan all documents in db to make sure they still exist
    if (!q.prepare(SELECT_ALL_DOCUMENTS_SQL)) {
        qWarning() << "ERROR: Cannot prepare sql for select all documents" << q.lastError();
        return;
    }

    if (!q.exec()) {
        qWarning() << "ERROR: Cannot exec sql for select all documents" << q.lastError();
        return;
    }

    transaction();

    while (q.next()) {
        int document_id = q.value(0).toInt();
        // Remove all chunks and documents to change the chunk size
        QSqlQuery query(m_db);
        if (!removeChunksByDocumentId(query, document_id)) {
            qWarning() << "ERROR: Cannot remove chunks of document_id" << document_id << query.lastError();
            return rollback();
        }

        if (!removeDocument(query, document_id)) {
            qWarning() << "ERROR: Cannot remove document_id" << document_id << query.lastError();
            return rollback();
        }
    }

    commit();

    m_chunkSize = chunkSize;
    addCurrentFolders();
    updateCollectionStatistics();
}

void Database::changeFileExtensions(const QStringList &extensions)
{
#if defined(DEBUG)
    qDebug() << "changeFileExtensions";
#endif

    m_scannedFileExtensions = extensions;

    if (cleanDB())
        updateCollectionStatistics();

    QSqlQuery q(m_db);
    QList<CollectionItem> collections;
    if (!selectAllFromCollections(q, &collections)) {
        qWarning() << "ERROR: Cannot select collections" << q.lastError();
        return;
    }

    for (const auto &i: std::as_const(collections)) {
        if (!i.forceIndexing)
            scanDocuments(i.folder_id, i.folder_path);
    }
}

void Database::directoryChanged(const QString &path)
{
#if defined(DEBUG)
    qDebug() << "directoryChanged" << path;
#endif

    // search for a collection that contains this folder (we watch subdirectories)
    int folder_id = -1;
    QDir dir(path);
    for (;;) {
        QSqlQuery q(m_db);
        if (!selectFolder(q, dir.path(), &folder_id)) {
            qWarning() << "ERROR: Cannot select folder from path" << dir.path() << q.lastError();
            return;
        }
        if (folder_id != -1)
            break;

        // check next parent
        if (!dir.cdUp()) {
            if (!dir.isRoot()) break; // folder removed
            Q_ASSERT(false);
            qWarning() << "ERROR: Watched folder does not exist in db" << path;
            m_watcher->removePath(path);
            return;
        }
    }

    // Clean the database
    if (cleanDB())
        updateCollectionStatistics();

    // Rescan the documents associated with the folder
    if (folder_id != -1)
        scanDocuments(folder_id, path);
}
