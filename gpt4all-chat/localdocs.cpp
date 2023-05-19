#include "localdocs.h"
#include "download.h"

#include <QTimer>
#include <QPdfDocument>

#define DEBUG
//#define DEBUG_EXAMPLE

#define LOCALDOCS_VERSION 0

const auto INSERT_CHUNK_SQL = QLatin1String(R"(
    insert into chunks(document_id, chunk_id, chunk_text,
        embedding_id, embedding_path) values(?, ?, ?, ?, ?);
    )");

const auto INSERT_CHUNK_FTS_SQL = QLatin1String(R"(
    insert into chunks_fts(document_id, chunk_id, chunk_text,
        embedding_id, embedding_path) values(?, ?, ?, ?, ?);
    )");

const auto DELETE_CHUNKS_SQL = QLatin1String(R"(
    DELETE FROM chunks WHERE document_id = ?;
    )");

const auto DELETE_CHUNKS_FTS_SQL = QLatin1String(R"(
    DELETE FROM chunks_fts WHERE document_id = ?;
    )");

const auto CHUNKS_SQL = QLatin1String(R"(
    create table chunks(document_id integer, chunk_id integer, chunk_text varchar,
        embedding_id integer, embedding_path varchar);
    )");

const auto FTS_CHUNKS_SQL = QLatin1String(R"(
    create virtual table chunks_fts using fts5(document_id unindexed, chunk_id unindexed, chunk_text,
        embedding_id unindexed, embedding_path unindexed, tokenize="trigram");
    )");

const auto SELECT_SQL = QLatin1String(R"(
    select chunks_fts.rowid, chunks_fts.document_id, chunks_fts.chunk_text
    from chunks_fts
    join documents ON chunks_fts.document_id = documents.id
    join folders ON documents.folder_id = folders.id
    join collections ON folders.id = collections.folder_id
    where chunks_fts match ? and collections.collection_name in (%1)
    order by bm25(chunks_fts) desc
    limit 3;
    )");

bool addChunk(QSqlQuery &q, int document_id, int chunk_id, const QString &chunk_text, int embedding_id,
    const QString &embedding_path)
{
    {
        if (!q.prepare(INSERT_CHUNK_SQL))
            return false;
        q.addBindValue(document_id);
        q.addBindValue(chunk_id);
        q.addBindValue(chunk_text);
        q.addBindValue(embedding_id);
        q.addBindValue(embedding_path);
        if (!q.exec())
            return false;
    }
    {
        if (!q.prepare(INSERT_CHUNK_FTS_SQL))
            return false;
        q.addBindValue(document_id);
        q.addBindValue(chunk_id);
        q.addBindValue(chunk_text);
        q.addBindValue(embedding_id);
        q.addBindValue(embedding_path);
        if (!q.exec())
            return false;
    }
    return true;
}

bool deleteChunksByDocumentId(QSqlQuery &q, int document_id)
{
    {
        if (!q.prepare(DELETE_CHUNKS_SQL))
            return false;
        q.addBindValue(document_id);
        if (!q.exec())
            return false;
    }

    {
        if (!q.prepare(DELETE_CHUNKS_FTS_SQL))
            return false;
        q.addBindValue(document_id);
        if (!q.exec())
            return false;
    }

    return true;
}

QStringList generateGrams(const QString &input, int N)
{
    // Remove common English punctuation using QRegularExpression
    QRegularExpression punctuation(R"([.,;:!?'"()\-])");
    QString cleanedInput = input;
    cleanedInput = cleanedInput.remove(punctuation);

    // Split the cleaned input into words using whitespace
    QStringList words = cleanedInput.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    N = qMin(words.size(), N);

    // Generate all possible N-grams
    QStringList ngrams;
    for (int i = 0; i < words.size() - (N - 1); ++i) {
        QStringList currentNgram;
        for (int j = 0; j < N; ++j) {
            currentNgram.append(words[i + j]);
        }
        ngrams.append("\"" + currentNgram.join(" ") + "\"");
    }
    return ngrams;
}

bool selectChunk(QSqlQuery &q, const QList<QString> &collection_names, const QString &chunk_text)
{
    for (int N = 5; N > 1; N--) {
        // first try trigrams
        QList<QString> text = generateGrams(chunk_text, N);
        QString orText = text.join(" OR ");
        qDebug() << "before" << chunk_text << "after" << orText;
        const QString collection_names_str = collection_names.join("', '");
        const QString formatted_query = SELECT_SQL.arg("'" + collection_names_str + "'");
        if (!q.prepare(formatted_query))
            return false;
        q.addBindValue(orText);
        bool success = q.exec();
        if (!success) return false;
        if (q.next()) {
            q.previous();
            return true;
        }
    }
    return true;
}

void printResults(QSqlQuery &q)
{
    while (q.next()) {
        int rowid = q.value(0).toInt();
        QString collection_name = q.value(1).toString();
        QString chunk_text = q.value(2).toString();

        qDebug() << "rowid:" << rowid
                 << "collection_name:" << collection_name
                 << "chunk_text:" << chunk_text;
    }
}

const auto INSERT_COLLECTION_SQL = QLatin1String(R"(
    insert into collections(collection_name, folder_id) values(?, ?);
    )");

const auto COLLECTIONS_SQL = QLatin1String(R"(
    create table collections(collection_name varchar, folder_id integer, unique(collection_name, folder_id));
    )");

const auto SELECT_FOLDERS_FROM_COLLECTIONS_SQL = QLatin1String(R"(
    select folder_id from collections where collection_name = ?;
    )");

bool addCollection(QSqlQuery &q, const QString &collection_name, int folder_id)
{
    if (!q.prepare(INSERT_COLLECTION_SQL))
        return false;
    q.addBindValue(collection_name);
    q.addBindValue(folder_id);
    return q.exec();
}

bool selectFoldersFromCollection(QSqlQuery &q, const QString &collection_name, QList<int> *folderIds) {
    if (!q.prepare(SELECT_FOLDERS_FROM_COLLECTIONS_SQL))
        return false;
    q.addBindValue(collection_name);
    if (!q.exec())
        return false;
    while (q.next())
        folderIds->append(q.value(0).toInt());
    return true;
}

const auto INSERT_FOLDERS_SQL = QLatin1String(R"(
    insert into folders(folder_path) values(?);
    )");

const auto DELETE_FOLDERS_SQL = QLatin1String(R"(
    delete from folders where folder_path = ?;
    )");

const auto SELECT_FOLDERS_SQL = QLatin1String(R"(
    select id from folders where folder_path = ?;
    )");

const auto FOLDERS_SQL = QLatin1String(R"(
    create table folders(id integer primary key, folder_path varchar unique);
    )");

bool addFolderToDB(QSqlQuery &q, const QString &folder_path, int *folder_id)
{
    if (!q.prepare(INSERT_FOLDERS_SQL))
        return false;
    q.addBindValue(folder_path);
    if (!q.exec())
        return false;
    *folder_id = q.lastInsertId().toInt();
    return true;
}

bool removeFolderFromDB(QSqlQuery &q, const QString &folder_path) {
    if (!q.prepare(DELETE_FOLDERS_SQL))
        return false;
    q.addBindValue(folder_path);
    return q.exec();
}

bool selectFolder(QSqlQuery &q, const QString &folder_path, int *id) {
    if (!q.prepare(SELECT_FOLDERS_SQL))
        return false;
    q.addBindValue(folder_path);
    if (!q.exec())
        return false;
    Q_ASSERT(q.size() < 2);
    if (q.next())
        *id = q.value(0).toInt();
    return true;
}

const auto INSERT_DOCUMENTS_SQL = QLatin1String(R"(
    insert into documents(folder_id, document_time, document_path) values(?, ?, ?);
    )");

const auto UPDATE_DOCUMENT_TIME_SQL = QLatin1String(R"(
    update documents set document_time = ? where id = ?;
    )");

const auto DOCUMENTS_SQL = QLatin1String(R"(
    create table documents(id integer primary key, folder_id integer, document_time integer, document_path varchar unique);
    )");

const auto SELECT_DOCUMENT_SQL = QLatin1String(R"(
    select id, document_time from documents where document_path = ?;
    )");

bool addDocument(QSqlQuery &q, int folder_id, qint64 document_time, const QString &document_path, int *document_id)
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

bool updateDocument(QSqlQuery &q, int id, qint64 document_time)
{
    if (!q.prepare(UPDATE_DOCUMENT_TIME_SQL))
        return false;
    q.addBindValue(id);
    q.addBindValue(document_time);
    return q.exec();
}

bool selectDocument(QSqlQuery &q, const QString &document_path, int *id, qint64 *document_time) {
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

QSqlError initDb()
{
    QString dbPath = Download::globalInstance()->downloadLocalModelsPath()
        + QString("localdocs_v%1.db").arg(LOCALDOCS_VERSION);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open())
        return db.lastError();

    QStringList tables = db.tables();
    if (tables.contains("chunks", Qt::CaseInsensitive))
        return QSqlError();

    QSqlQuery q;
    if (!q.exec(CHUNKS_SQL))
        return q.lastError();

    if (!q.exec(FTS_CHUNKS_SQL))
        return q.lastError();

    if (!q.exec(COLLECTIONS_SQL))
        return q.lastError();

    if (!q.exec(FOLDERS_SQL))
        return q.lastError();

    if (!q.exec(DOCUMENTS_SQL))
        return q.lastError();

#if defined(DEBUG_EXAMPLE)
    // Add a folder
    QString folder_path = "/example/folder";
    int folder_id;
    if (!addFolderToDB(q, folder_path, &folder_id)) {
        qDebug() << "Error adding folder:" << q.lastError().text();
        return q.lastError();
    }

    // Add a collection
    QString collection_name = "Example Collection";
    if (!addCollection(q, collection_name, folder_id)) {
        qDebug() << "Error adding collection:" << q.lastError().text();
        return q.lastError();
    }

    // Add a document
    int document_time = 123456789;
    int document_id;
    QString document_path = "/example/folder/document1.txt";
    if (!addDocument(q, folder_id, document_time, document_path, &document_id)) {
        qDebug() << "Error adding document:" << q.lastError().text();
        return q.lastError();
    }

    // Add chunks to the document
    QString chunk_text1 = "This is an example chunk.";
    QString chunk_text2 = "Another example chunk.";
    QString embedding_path = "/example/embeddings/embedding1.bin";
    int embedding_id = 1;

    if (!addChunk(q, document_id, 1, chunk_text1, embedding_id, embedding_path) ||
        !addChunk(q, document_id, 2, chunk_text2, embedding_id, embedding_path)) {
        qDebug() << "Error adding chunks:" << q.lastError().text();
        return q.lastError();
    }

    // Perform a search
    QList<QString> collection_names = {collection_name};
    QString search_text = "example";
    if (!selectChunk(q, collection_names, search_text)) {
        qDebug() << "Error selecting chunks:" << q.lastError().text();
        return q.lastError();
    }

    // Print the results
    printResults(q);
#endif

    return QSqlError();
}

Database::Database()
    : QObject(nullptr)
    , m_watcher(new QFileSystemWatcher(this))
{
    moveToThread(&m_dbThread);
    connect(&m_dbThread, &QThread::started, this, &Database::start);
    m_dbThread.setObjectName("database");
    m_dbThread.start();
}

void Database::handleDocumentErrorAndScheduleNext(const QString &errorMessage,
    int document_id, const QString &document_path, const QSqlError &error)
{
    qWarning() << errorMessage << document_id << document_path << error.text();
    if (!m_docsToScan.isEmpty())
        QTimer::singleShot(0, this, &Database::scanQueue);
}

void Database::chunkStream(QTextStream &stream, int document_id)
{
    QString text = stream.readAll();
    int chunkSize = 256;
    int overlap = 25;
    int chunk_id = 0;

    for (int i = 0; i + chunkSize < text.length(); i += (chunkSize - overlap)) {
        QString chunk = text.mid(i, chunkSize);
        QSqlQuery q;
        if (!addChunk(q,
            document_id,
            ++chunk_id,
            chunk,
            0 /*embedding_id*/,
            QString() /*embedding_path*/
        )) {
            qWarning() << "ERROR: Could not insert chunk into db" << q.lastError();
        }
    }
}

void Database::scanQueue()
{
    if (m_docsToScan.isEmpty())
        return;

    const DocumentInfo info = m_docsToScan.dequeue();
    const int folder_id = info.folder;
    const qint64 document_time = info.doc.fileTime(QFile::FileModificationTime).toMSecsSinceEpoch();
    const QString document_path = info.doc.canonicalFilePath();

#if defined(DEBUG)
    qDebug() << "scanning document" << document_path;
#endif

    // Check and see if we already have this document
    QSqlQuery q;
    int existing_id = -1;
    qint64 existing_time = -1;
    if (!selectDocument(q, document_path, &existing_id, &existing_time)) {
        return handleDocumentErrorAndScheduleNext("ERROR: Cannot select document",
            existing_id, document_path, q.lastError());
    }

    // If we have the document, we need to compare the last modification time and if it is newer
    // we must rescan the document, otherwise return
    if (existing_id != -1) {
        Q_ASSERT(existing_time != -1);
        if (document_time == existing_time) {
            // No need to rescan, but we do have to schedule next
            if (!m_docsToScan.isEmpty()) QTimer::singleShot(0, this, &Database::scanQueue);
            return;
        } else {
            if (!deleteChunksByDocumentId(q, existing_id)) {
                return handleDocumentErrorAndScheduleNext("ERROR: Cannot delete chunks of document",
                    existing_id, document_path, q.lastError());
            }
        }
    }

    // Update the document_time for an existing document, or add it for the first time now
    int document_id = existing_id;
    if (document_id != -1) {
        if (!updateDocument(q, document_id, document_time)) {
            return handleDocumentErrorAndScheduleNext("ERROR: Could not update document_time",
                document_id, document_path, q.lastError());
        }
    } else {
        if (!addDocument(q, folder_id, document_time, document_path, &document_id)) {
            return handleDocumentErrorAndScheduleNext("ERROR: Could not add document",
                document_id, document_path, q.lastError());
        }
    }

    QElapsedTimer timer;
    timer.start();

    QSqlDatabase::database().transaction();
    Q_ASSERT(document_id != -1);
    if (info.doc.suffix() == QLatin1String("pdf")) {
        QPdfDocument doc;
        if (QPdfDocument::Error::None != doc.load(info.doc.canonicalFilePath())) {
            return handleDocumentErrorAndScheduleNext("ERROR: Could not load pdf",
                document_id, document_path, q.lastError());
            return;
        }
        QString text;
        for (int i = 0; i < doc.pageCount(); ++i) {
            const QPdfSelection selection = doc.getAllText(i);
            text.append(selection.text());
        }
        QTextStream stream(&text);
        chunkStream(stream, document_id);
    } else {
        QFile file(document_path);
        if (!file.open( QIODevice::ReadOnly)) {
            return handleDocumentErrorAndScheduleNext("ERROR: Cannot open file for scanning",
                                                      existing_id, document_path, q.lastError());
        }
        QTextStream stream(&file);
        chunkStream(stream, document_id);
        file.close();
    }
    QSqlDatabase::database().commit();

#if defined(DEBUG)
    qDebug() << "chunking" << document_path << "took" << timer.elapsed() << "ms";
#endif

    if (!m_docsToScan.isEmpty()) QTimer::singleShot(0, this, &Database::scanQueue);
}

void Database::scanDocuments(int folder_id, const QString &folder_path)
{
#if defined(DEBUG)
    qDebug() << "scanning folder for documents" << folder_path;
#endif

    QDir dir(folder_path);
    Q_ASSERT(dir.exists());
    Q_ASSERT(dir.isReadable());
    QDirIterator it(folder_path, QDir::Readable | QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        fileInfo.setCaching(false);
        if (fileInfo.isDir()) {
            addFolderToWatch(fileInfo.canonicalFilePath());
            continue;
        }

        if (fileInfo.suffix() != QLatin1String("pdf")
            && fileInfo.suffix() != QLatin1String("txt")) {
            continue;
        }

        DocumentInfo info;
        info.folder = folder_id;
        info.doc = fileInfo;
        m_docsToScan.enqueue(info);
    }
    emit docsToScanChanged();
}

void Database::start()
{
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &Database::directoryChanged);
    connect(this, &Database::docsToScanChanged, this, &Database::scanQueue);
    if (!QSqlDatabase::drivers().contains("QSQLITE")) {
        qWarning() << "ERROR: missing sqllite driver";
    } else {
        QSqlError err = initDb();
        if (err.type() != QSqlError::NoError)
            qWarning() << "ERROR: initializing db" << err.text();
    }
}

void Database::addFolder(const QString &collection, const QString &path)
{
    QFileInfo info(path);
    if (!info.exists() || !info.isReadable()) {
        qWarning() << "ERROR: Cannot add folder that doesn't exist or not readable" << path;
        return;
    }

    QSqlQuery q;
    int folder_id = -1;

    // See if the folder exists in the db
    if (!selectFolder(q, path, &folder_id)) {
        qWarning() << "ERROR: Cannot select folder from path" << path << q.lastError();
        return;
    }

    // Add the folder
    if (folder_id == -1 && !addFolderToDB(q, path, &folder_id)) {
        qWarning() << "ERROR: Cannot add folder to db with path" << path << q.lastError();
        return;
    }

    Q_ASSERT(folder_id != -1);

    // See if the folder has already been added to the collection
    QList<int> folders;
    if (!selectFoldersFromCollection(q, collection, &folders)) {
        qWarning() << "ERROR: Cannot select folders from collections" << collection << q.lastError();
        return;
    }

    if (!folders.contains(folder_id) && !addCollection(q, collection, folder_id)) {
        qWarning() << "ERROR: Cannot add folder to collection" << collection << path << q.lastError();
        return;
    }

    if (!addFolderToWatch(path))
        return;

    scanDocuments(folder_id, path);
}

void Database::removeFolder(const QString &collection, const QString &path)
{
#if defined(DEBUG)
    qDebug() << "removeFolder" << path;
#endif

    // FIXME: Determine if the folder is used by more than one collection

    // FIXME: If not, then delete all chunks and documents associated with it

    // FIXME: Remove it from the collections

    removeFolderFromWatch(path);
}

bool Database::addFolderToWatch(const QString &path)
{
#if defined(DEBUG)
    qDebug() << "addFolderToWatch" << path;
#endif

    if (!m_watcher->addPath(path)) {
        qWarning() << "ERROR: Cannot add path to file watcher:" << path;
        return false;
    }
    return true;
}

bool Database::removeFolderFromWatch(const QString &path)
{
#if defined(DEBUG)
    qDebug() << "removeFolderFromWatch" << path;
#endif
    if (!m_watcher->removePath(path)) {
        qWarning() << "ERROR: Cannot remove path from file watcher:" << path;
        return false;
    }
    return true;
}

void Database::retrieveFromDB(const QList<QString> &collections, const QString &text)
{
#if defined(DEBUG)
    qDebug() << "retrieveFromDB" << collections << text;
#endif

    QSqlQuery q;
    if (!selectChunk(q, collections, text)) {
        qDebug() << "ERROR: selecting chunks:" << q.lastError().text();
        return;
    }

    QList<QString> results;
    while (q.next()) {
        int rowid = q.value(0).toInt();
        QString collection_name = q.value(1).toString();
        QString chunk_text = q.value(2).toString();
        results.append(chunk_text);
#if defined(DEBUG)
        qDebug() << "retrieve rowid:" << rowid
                 << "collection_name:" << collection_name
                 << "chunk_text:" << chunk_text;
#endif
    }

    emit retrieveResult(results);
}

void Database::directoryChanged(const QString &path)
{
#if defined(DEBUG)
    qDebug() << "directoryChanged" << path;
#endif

    QSqlQuery q;
    int folder_id = -1;

    // Lookup the folder_id in the db
    if (!selectFolder(q, path, &folder_id)) {
        qWarning() << "ERROR: Cannot select folder from path" << path << q.lastError();
        return;
    }

    // If we don't have a folder_id in the db, then something bad has happened
    Q_ASSERT(folder_id != -1);
    if (folder_id == -1) {
        qWarning() << "ERROR: Watched folder does not exist in db" << path;
        m_watcher->removePath(path);
        return;
    }

    // Rescan the documents associated with the folder
    scanDocuments(folder_id, path);
}

class MyLocalDocs: public LocalDocs { };
Q_GLOBAL_STATIC(MyLocalDocs, localDocsInstance)
LocalDocs *LocalDocs::globalInstance()
{
    return localDocsInstance();
}

LocalDocs::LocalDocs()
    : QObject(nullptr)
    , m_database(new Database)
    , m_retrieveInProgress(false)
{
    connect(this, &LocalDocs::requestAddFolder, m_database,
        &Database::addFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestRemoveFolder, m_database,
        &Database::removeFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestRetrieveFromDB, m_database,
        &Database::retrieveFromDB, Qt::QueuedConnection);
    connect(m_database, &Database::retrieveResult, this,
        &LocalDocs::retrieveResult, Qt::QueuedConnection);

    addFolder("localdocs", "/home/atreat/dev/large_language_models/localdocs");
}

void LocalDocs::addFolder(const QString &collection, const QString &path)
{
    emit requestAddFolder(collection, path);
}

void LocalDocs::removeFolder(const QString &collection, const QString &path)
{
    emit requestRemoveFolder(collection, path);
}

void LocalDocs::requestRetrieve(const QList<QString> &collections, const QString &text)
{
    m_retrieveInProgress = true;
    m_retrieveResult = QList<QString>();
    emit retrieveInProgressChanged();
    emit requestRetrieveFromDB(collections, text);
}

void LocalDocs::retrieveResult(const QList<QString> &result)
{
    m_retrieveInProgress = false;
    m_retrieveResult = result;
    emit receivedResult();
    emit retrieveInProgressChanged();
}
