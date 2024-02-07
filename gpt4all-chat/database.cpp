#include "database.h"
#include "mysettings.h"
#include "embllm.h"
#include "embeddings.h"

#include <QTimer>
#include <QPdfDocument>

//#define DEBUG
//#define DEBUG_EXAMPLE

#define LOCALDOCS_VERSION 1

const auto INSERT_CHUNK_SQL = QLatin1String(R"(
    insert into chunks(document_id, chunk_text,
        file, title, author, subject, keywords, page, line_from, line_to)
        values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )");

const auto INSERT_CHUNK_FTS_SQL = QLatin1String(R"(
    insert into chunks_fts(document_id, chunk_id, chunk_text,
        file, title, author, subject, keywords, page, line_from, line_to)
        values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )");

const auto DELETE_CHUNKS_SQL = QLatin1String(R"(
    delete from chunks WHERE document_id = ?;
    )");

const auto DELETE_CHUNKS_FTS_SQL = QLatin1String(R"(
    delete from chunks_fts WHERE document_id = ?;
    )");

const auto CHUNKS_SQL = QLatin1String(R"(
    create table chunks(document_id integer, chunk_id integer primary key autoincrement, chunk_text varchar,
        file varchar, title varchar, author varchar, subject varchar, keywords varchar,
        page integer, line_from integer, line_to integer);
    )");

const auto FTS_CHUNKS_SQL = QLatin1String(R"(
    create virtual table chunks_fts using fts5(document_id unindexed, chunk_id unindexed, chunk_text,
        file, title, author, subject, keywords, page, line_from, line_to, tokenize="trigram");
    )");

const auto SELECT_CHUNKS_BY_DOCUMENT_SQL = QLatin1String(R"(
    select chunk_id from chunks WHERE document_id = ?;
    )");

const auto SELECT_CHUNKS_SQL = QLatin1String(R"(
    select chunks.chunk_id, documents.document_time,
        chunks.chunk_text, chunks.file, chunks.title, chunks.author, chunks.page,
        chunks.line_from, chunks.line_to
    from chunks
    join documents ON chunks.document_id = documents.id
    join folders ON documents.folder_id = folders.id
    join collections ON folders.id = collections.folder_id
    where chunks.chunk_id in (%1) and collections.collection_name in (%2);
)");

const auto SELECT_NGRAM_SQL = QLatin1String(R"(
    select chunks_fts.chunk_id, documents.document_time,
        chunks_fts.chunk_text, chunks_fts.file, chunks_fts.title, chunks_fts.author, chunks_fts.page,
        chunks_fts.line_from, chunks_fts.line_to
    from chunks_fts
    join documents ON chunks_fts.document_id = documents.id
    join folders ON documents.folder_id = folders.id
    join collections ON folders.id = collections.folder_id
    where chunks_fts match ? and collections.collection_name in (%1)
    order by bm25(chunks_fts)
    limit %2;
    )");

bool addChunk(QSqlQuery &q, int document_id, const QString &chunk_text,
    const QString &file, const QString &title, const QString &author, const QString &subject, const QString &keywords,
    int page, int from, int to, int *chunk_id)
{
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
        if (!q.exec())
            return false;
    }
    if (!q.exec("select last_insert_rowid();"))
        return false;
    if (!q.next())
        return false;
    *chunk_id = q.value(0).toInt();
    {
        if (!q.prepare(INSERT_CHUNK_FTS_SQL))
            return false;
        q.addBindValue(document_id);
        q.addBindValue(*chunk_id);
        q.addBindValue(chunk_text);
        q.addBindValue(file);
        q.addBindValue(title);
        q.addBindValue(author);
        q.addBindValue(subject);
        q.addBindValue(keywords);
        q.addBindValue(page);
        q.addBindValue(from);
        q.addBindValue(to);
        if (!q.exec())
            return false;
    }
    return true;
}

bool removeChunksByDocumentId(QSqlQuery &q, int document_id)
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
    static QRegularExpression punctuation(R"([.,;:!?'"()\-])");
    QString cleanedInput = input;
    cleanedInput = cleanedInput.remove(punctuation);

    // Split the cleaned input into words using whitespace
    static QRegularExpression spaces("\\s+");
    QStringList words = cleanedInput.split(spaces, Qt::SkipEmptyParts);
    N = qMin(words.size(), N);

    // Generate all possible N-grams
    QStringList ngrams;
    for (int i = 0; i < words.size() - (N - 1); ++i) {
        QStringList currentNgram;
        for (int j = 0; j < N; ++j) {
            currentNgram.append("\"" + words[i + j] + "\"");
        }
        ngrams.append("NEAR(" + currentNgram.join(" ") + ", " + QString::number(N) + ")");
    }
    return ngrams;
}

bool selectChunk(QSqlQuery &q, const QList<QString> &collection_names, const std::vector<qint64> &chunk_ids, int retrievalSize)
{
    QString chunk_ids_str = QString::number(chunk_ids[0]);
    for (size_t i = 1; i < chunk_ids.size(); ++i)
        chunk_ids_str += "," + QString::number(chunk_ids[i]);
    const QString collection_names_str = collection_names.join("', '");
    const QString formatted_query = SELECT_CHUNKS_SQL.arg(chunk_ids_str).arg("'" + collection_names_str + "'");
    if (!q.prepare(formatted_query))
        return false;
    return q.exec();
}

bool selectChunk(QSqlQuery &q, const QList<QString> &collection_names, const QString &chunk_text, int retrievalSize)
{
    static QRegularExpression spaces("\\s+");
    const int N_WORDS = chunk_text.split(spaces).size();
    for (int N = N_WORDS; N > 2; N--) {
        // first try trigrams
        QList<QString> text = generateGrams(chunk_text, N);
        QString orText = text.join(" OR ");
        const QString collection_names_str = collection_names.join("', '");
        const QString formatted_query = SELECT_NGRAM_SQL.arg("'" + collection_names_str + "'").arg(QString::number(retrievalSize));
        if (!q.prepare(formatted_query))
            return false;
        q.addBindValue(orText);
        bool success = q.exec();
        if (!success) return false;
        if (q.next()) {
#if defined(DEBUG)
            qDebug() << "hit on" << N << "before" << chunk_text << "after" << orText;
#endif
            q.previous();
            return true;
        }
    }
    return true;
}

const auto INSERT_COLLECTION_SQL = QLatin1String(R"(
    insert into collections(collection_name, folder_id) values(?, ?);
    )");

const auto DELETE_COLLECTION_SQL = QLatin1String(R"(
    delete from collections where collection_name = ? and folder_id = ?;
    )");

const auto COLLECTIONS_SQL = QLatin1String(R"(
    create table collections(collection_name varchar, folder_id integer, unique(collection_name, folder_id));
    )");

const auto SELECT_FOLDERS_FROM_COLLECTIONS_SQL = QLatin1String(R"(
    select folder_id from collections where collection_name = ?;
    )");

const auto SELECT_COLLECTIONS_FROM_FOLDER_SQL = QLatin1String(R"(
    select collection_name from collections where folder_id = ?;
    )");

const auto SELECT_COLLECTIONS_SQL = QLatin1String(R"(
    select c.collection_name, f.folder_path, f.id
    from collections c
    join folders f on c.folder_id = f.id
    order by c.collection_name asc, f.folder_path asc;
    )");

bool addCollection(QSqlQuery &q, const QString &collection_name, int folder_id)
{
    if (!q.prepare(INSERT_COLLECTION_SQL))
        return false;
    q.addBindValue(collection_name);
    q.addBindValue(folder_id);
    return q.exec();
}

bool removeCollection(QSqlQuery &q, const QString &collection_name, int folder_id)
{
    if (!q.prepare(DELETE_COLLECTION_SQL))
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

bool selectCollectionsFromFolder(QSqlQuery &q, int folder_id, QList<QString> *collections) {
    if (!q.prepare(SELECT_COLLECTIONS_FROM_FOLDER_SQL))
        return false;
    q.addBindValue(folder_id);
    if (!q.exec())
        return false;
    while (q.next())
        collections->append(q.value(0).toString());
    return true;
}

bool selectAllFromCollections(QSqlQuery &q, QList<CollectionItem> *collections) {
    if (!q.prepare(SELECT_COLLECTIONS_SQL))
        return false;
    if (!q.exec())
        return false;
    while (q.next()) {
        CollectionItem i;
        i.collection = q.value(0).toString();
        i.folder_path = q.value(1).toString();
        i.folder_id = q.value(2).toInt();
        i.indexing = false;
        i.installed = true;
        collections->append(i);
    }
    return true;
}

const auto INSERT_FOLDERS_SQL = QLatin1String(R"(
    insert into folders(folder_path) values(?);
    )");

const auto DELETE_FOLDERS_SQL = QLatin1String(R"(
    delete from folders where id = ?;
    )");

const auto SELECT_FOLDERS_FROM_PATH_SQL = QLatin1String(R"(
    select id from folders where folder_path = ?;
    )");

const auto SELECT_FOLDERS_FROM_ID_SQL = QLatin1String(R"(
    select folder_path from folders where id = ?;
    )");

const auto SELECT_ALL_FOLDERPATHS_SQL = QLatin1String(R"(
    select folder_path from folders;
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

bool removeFolderFromDB(QSqlQuery &q, int folder_id) {
    if (!q.prepare(DELETE_FOLDERS_SQL))
        return false;
    q.addBindValue(folder_id);
    return q.exec();
}

bool selectFolder(QSqlQuery &q, const QString &folder_path, int *id) {
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

bool selectFolder(QSqlQuery &q, int id, QString *folder_path) {
    if (!q.prepare(SELECT_FOLDERS_FROM_ID_SQL))
        return false;
    q.addBindValue(id);
    if (!q.exec())
        return false;
    Q_ASSERT(q.size() < 2);
    if (q.next())
        *folder_path = q.value(0).toString();
    return true;
}

bool selectAllFolderPaths(QSqlQuery &q, QList<QString> *folder_paths) {
    if (!q.prepare(SELECT_ALL_FOLDERPATHS_SQL))
        return false;
    if (!q.exec())
        return false;
    while (q.next())
        folder_paths->append(q.value(0).toString());
    return true;
}

const auto INSERT_DOCUMENTS_SQL = QLatin1String(R"(
    insert into documents(folder_id, document_time, document_path) values(?, ?, ?);
    )");

const auto UPDATE_DOCUMENT_TIME_SQL = QLatin1String(R"(
    update documents set document_time = ? where id = ?;
    )");

const auto DELETE_DOCUMENTS_SQL = QLatin1String(R"(
    delete from documents where id = ?;
    )");

const auto DOCUMENTS_SQL = QLatin1String(R"(
    create table documents(id integer primary key, folder_id integer, document_time integer, document_path varchar unique);
    )");

const auto SELECT_DOCUMENT_SQL = QLatin1String(R"(
    select id, document_time from documents where document_path = ?;
    )");

const auto SELECT_DOCUMENTS_SQL = QLatin1String(R"(
    select id from documents where folder_id = ?;
    )");

const auto SELECT_ALL_DOCUMENTS_SQL = QLatin1String(R"(
    select id, document_path from documents;
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

bool removeDocument(QSqlQuery &q, int document_id) {
    if (!q.prepare(DELETE_DOCUMENTS_SQL))
        return false;
    q.addBindValue(document_id);
    return q.exec();
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

bool selectDocuments(QSqlQuery &q, int folder_id, QList<int> *documentIds) {
    if (!q.prepare(SELECT_DOCUMENTS_SQL))
        return false;
    q.addBindValue(folder_id);
    if (!q.exec())
        return false;
    while (q.next())
        documentIds->append(q.value(0).toInt());
    return true;
}

QSqlError initDb()
{
    QString dbPath = MySettings::globalInstance()->modelPath()
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

    CollectionItem i;
    i.collection = collection_name;
    i.folder_path = folder_path;
    i.folder_id = folder_id;
    emit addCollectionItem(i);

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
    QString file = "document1.txt";
    QString title;
    QString author;
    QString subject;
    QString keywords;
    int page = -1;
    int from = -1;
    int to = -1;;
    int embedding_id = 1;

    if (!addChunk(q, document_id, 1, chunk_text1, file, title, author, subject, keywords, page, from, to, embedding_id, embedding_path) ||
        !addChunk(q, document_id, 2, chunk_text2, file, title, author, subject, keywords, page, from, to, embedding_id, embedding_path)) {
        qDebug() << "Error adding chunks:" << q.lastError().text();
        return q.lastError();
    }

    // Perform a search
    QList<QString> collection_names = {collection_name};
    QString search_text = "example";
    if (!selectChunk(q, collection_names, search_text, 3)) {
        qDebug() << "Error selecting chunks:" << q.lastError().text();
        return q.lastError();
    }
#endif

    return QSqlError();
}

Database::Database(int chunkSize)
    : QObject(nullptr)
    , m_watcher(new QFileSystemWatcher(this))
    , m_chunkSize(chunkSize)
    , m_embLLM(new EmbeddingLLM)
    , m_embeddings(new Embeddings(this))
{
    moveToThread(&m_dbThread);
    connect(&m_dbThread, &QThread::started, this, &Database::start);
    m_dbThread.setObjectName("database");
    m_dbThread.start();
}

Database::~Database()
{
    m_dbThread.quit();
    m_dbThread.wait();
}

void Database::scheduleNext(int folder_id, size_t countForFolder)
{
    emit updateCurrentDocsToIndex(folder_id, countForFolder);
    if (!countForFolder) {
        emit updateIndexing(folder_id, false);
        emit updateInstalled(folder_id, true);
    }
    if (!m_docsToScan.isEmpty())
        QTimer::singleShot(0, this, &Database::scanQueue);
}

void Database::handleDocumentError(const QString &errorMessage,
    int document_id, const QString &document_path, const QSqlError &error)
{
    qWarning() << errorMessage << document_id << document_path << error.text();
}

size_t Database::chunkStream(QTextStream &stream, int folder_id, int document_id, const QString &file,
    const QString &title, const QString &author, const QString &subject, const QString &keywords, int page,
    int maxChunks)
{
    int charCount = 0;
    int line_from = -1;
    int line_to = -1;
    QList<QString> words;
    int chunks = 0;

    QVector<EmbeddingChunk> chunkList;

    while (!stream.atEnd()) {
        QString word;
        stream >> word;
        charCount += word.length();
        words.append(word);
        if (charCount + words.size() - 1 >= m_chunkSize || stream.atEnd()) {
            const QString chunk = words.join(" ");
            QSqlQuery q;
            int chunk_id = 0;
            if (!addChunk(q,
                document_id,
                chunk,
                file,
                title,
                author,
                subject,
                keywords,
                page,
                line_from,
                line_to,
                &chunk_id
            )) {
                qWarning() << "ERROR: Could not insert chunk into db" << q.lastError();
            }

#if 1
            EmbeddingChunk toEmbed;
            toEmbed.folder_id = folder_id;
            toEmbed.chunk_id = chunk_id;
            toEmbed.chunk = chunk;
            chunkList << toEmbed;
            if (chunkList.count() == 100) {
                m_embLLM->generateAsyncEmbeddings(chunkList);
                emit updateTotalEmbeddingsToIndex(folder_id, 100);
                chunkList.clear();
            }
#else
            const std::vector<float> result = m_embLLM->generateEmbeddings(chunk);
            if (!m_embeddings->add(result, chunk_id))
                qWarning() << "ERROR: Cannot add point to embeddings index";
#endif

            ++chunks;

            words.clear();
            charCount = 0;

            if (maxChunks > 0 && chunks == maxChunks)
                break;
        }
    }

    if (!chunkList.isEmpty()) {
        m_embLLM->generateAsyncEmbeddings(chunkList);
        emit updateTotalEmbeddingsToIndex(folder_id, chunkList.count());
        chunkList.clear();
    }

    return stream.pos();
}

void Database::handleEmbeddingsGenerated(const QVector<EmbeddingResult> &embeddings)
{
    if (embeddings.isEmpty())
        return;

    int folder_id = 0;
    for (auto e : embeddings) {
        folder_id = e.folder_id;
        if (!m_embeddings->add(e.embedding, e.chunk_id))
            qWarning() << "ERROR: Cannot add point to embeddings index";
    }
    emit updateCurrentEmbeddingsToIndex(folder_id, embeddings.count());
    m_embeddings->save();
}

void Database::handleErrorGenerated(int folder_id, const QString &error)
{
    emit updateError(folder_id, error);
}

void Database::removeEmbeddingsByDocumentId(int document_id)
{
    QSqlQuery q;

    if (!q.prepare(SELECT_CHUNKS_BY_DOCUMENT_SQL)) {
        qWarning() << "ERROR: Cannot prepare sql for select chunks by document" << q.lastError();
        return;
    }

    q.addBindValue(document_id);

    if (!q.exec()) {
        qWarning() << "ERROR: Cannot exec sql for select chunks by document" << q.lastError();
        return;
    }

    while (q.next()) {
        const int chunk_id = q.value(0).toInt();
        m_embeddings->remove(chunk_id);
    }
    m_embeddings->save();
}

size_t Database::countOfDocuments(int folder_id) const
{
    if (!m_docsToScan.contains(folder_id))
        return 0;
    return m_docsToScan.value(folder_id).size();
}

size_t Database::countOfBytes(int folder_id) const
{
    if (!m_docsToScan.contains(folder_id))
        return 0;
    size_t totalBytes = 0;
    const QQueue<DocumentInfo> &docs = m_docsToScan.value(folder_id);
    for (const DocumentInfo &f : docs)
        totalBytes += f.doc.size();
    return totalBytes;
}

DocumentInfo Database::dequeueDocument()
{
    Q_ASSERT(!m_docsToScan.isEmpty());
    const int firstKey = m_docsToScan.firstKey();
    QQueue<DocumentInfo> &queue = m_docsToScan[firstKey];
    Q_ASSERT(!queue.isEmpty());
    DocumentInfo result = queue.dequeue();
    if (queue.isEmpty())
        m_docsToScan.remove(firstKey);
    return result;
}

void Database::removeFolderFromDocumentQueue(int folder_id)
{
    if (!m_docsToScan.contains(folder_id))
        return;
    m_docsToScan.remove(folder_id);
    emit removeFolderById(folder_id);
    emit docsToScanChanged();
}

void Database::enqueueDocumentInternal(const DocumentInfo &info, bool prepend)
{
    const int key = info.folder;
    if (!m_docsToScan.contains(key))
        m_docsToScan[key] = QQueue<DocumentInfo>();
    if (prepend)
        m_docsToScan[key].prepend(info);
    else
        m_docsToScan[key].enqueue(info);
}

void Database::enqueueDocuments(int folder_id, const QVector<DocumentInfo> &infos)
{
    for (int i = 0; i < infos.size(); ++i)
        enqueueDocumentInternal(infos[i]);
    const size_t count = countOfDocuments(folder_id);
    emit updateCurrentDocsToIndex(folder_id, count);
    emit updateTotalDocsToIndex(folder_id, count);
    const size_t bytes = countOfBytes(folder_id);
    emit updateCurrentBytesToIndex(folder_id, bytes);
    emit updateTotalBytesToIndex(folder_id, bytes);
    emit docsToScanChanged();
}

void Database::scanQueue()
{
    if (m_docsToScan.isEmpty())
        return;

    DocumentInfo info = dequeueDocument();
    const size_t countForFolder = countOfDocuments(info.folder);
    const int folder_id = info.folder;

    // Update info
    info.doc.stat();

    // If the doc has since been deleted or no longer readable, then we schedule more work and return
    // leaving the cleanup for the cleanup handler
    if (!info.doc.exists() || !info.doc.isReadable()) {
        return scheduleNext(folder_id, countForFolder);
    }

    const qint64 document_time = info.doc.fileTime(QFile::FileModificationTime).toMSecsSinceEpoch();
    const QString document_path = info.doc.canonicalFilePath();
    const bool currentlyProcessing = info.currentlyProcessing;

    // Check and see if we already have this document
    QSqlQuery q;
    int existing_id = -1;
    qint64 existing_time = -1;
    if (!selectDocument(q, document_path, &existing_id, &existing_time)) {
        handleDocumentError("ERROR: Cannot select document",
            existing_id, document_path, q.lastError());
        return scheduleNext(folder_id, countForFolder);
    }

    // If we have the document, we need to compare the last modification time and if it is newer
    // we must rescan the document, otherwise return
    if (existing_id != -1 && !currentlyProcessing) {
        Q_ASSERT(existing_time != -1);
        if (document_time == existing_time) {
            // No need to rescan, but we do have to schedule next
            return scheduleNext(folder_id, countForFolder);
        } else {
            removeEmbeddingsByDocumentId(existing_id);
            if (!removeChunksByDocumentId(q, existing_id)) {
                handleDocumentError("ERROR: Cannot remove chunks of document",
                    existing_id, document_path, q.lastError());
                return scheduleNext(folder_id, countForFolder);
            }
        }
    }

    // Update the document_time for an existing document, or add it for the first time now
    int document_id = existing_id;
    if (!currentlyProcessing) {
        if (document_id != -1) {
            if (!updateDocument(q, document_id, document_time)) {
                handleDocumentError("ERROR: Could not update document_time",
                    document_id, document_path, q.lastError());
                return scheduleNext(folder_id, countForFolder);
            }
        } else {
            if (!addDocument(q, folder_id, document_time, document_path, &document_id)) {
                handleDocumentError("ERROR: Could not add document",
                    document_id, document_path, q.lastError());
                return scheduleNext(folder_id, countForFolder);
            }
        }
    }

    QSqlDatabase::database().transaction();
    Q_ASSERT(document_id != -1);
    if (info.isPdf()) {
        QPdfDocument doc;
        if (QPdfDocument::Error::None != doc.load(info.doc.canonicalFilePath())) {
            handleDocumentError("ERROR: Could not load pdf",
                document_id, document_path, q.lastError());
            return scheduleNext(folder_id, countForFolder);
        }
        const size_t bytes = info.doc.size();
        const size_t bytesPerPage = std::floor(bytes / doc.pageCount());
        const int pageIndex = info.currentPage;
#if defined(DEBUG)
        qDebug() << "scanning page" << pageIndex << "of" << doc.pageCount() << document_path;
#endif
        const QPdfSelection selection = doc.getAllText(pageIndex);
        QString text = selection.text();
        QTextStream stream(&text);
        chunkStream(stream, info.folder, document_id, info.doc.fileName(),
            doc.metaData(QPdfDocument::MetaDataField::Title).toString(),
            doc.metaData(QPdfDocument::MetaDataField::Author).toString(),
            doc.metaData(QPdfDocument::MetaDataField::Subject).toString(),
            doc.metaData(QPdfDocument::MetaDataField::Keywords).toString(),
            pageIndex + 1
        );
        emit subtractCurrentBytesToIndex(info.folder, bytesPerPage);
        if (info.currentPage < doc.pageCount()) {
            info.currentPage += 1;
            info.currentlyProcessing = true;
            enqueueDocumentInternal(info, true /*prepend*/);
            return scheduleNext(folder_id, countForFolder + 1);
        } else {
            emit subtractCurrentBytesToIndex(info.folder, bytes - (bytesPerPage * doc.pageCount()));
        }
    } else {
        QFile file(document_path);
        if (!file.open(QIODevice::ReadOnly)) {
            handleDocumentError("ERROR: Cannot open file for scanning",
                                existing_id, document_path, q.lastError());
            return scheduleNext(folder_id, countForFolder);
        }

        const size_t bytes = info.doc.size();
        QTextStream stream(&file);
        const size_t byteIndex = info.currentPosition;
        if (!stream.seek(byteIndex)) {
            handleDocumentError("ERROR: Cannot seek to pos for scanning",
                                existing_id, document_path, q.lastError());
            return scheduleNext(folder_id, countForFolder);
        }
#if defined(DEBUG)
        qDebug() << "scanning byteIndex" << byteIndex << "of" << bytes << document_path;
#endif
        int pos = chunkStream(stream, info.folder, document_id, info.doc.fileName(), QString() /*title*/, QString() /*author*/,
            QString() /*subject*/, QString() /*keywords*/, -1 /*page*/, 100 /*maxChunks*/);
        file.close();
        const size_t bytesChunked = pos - byteIndex;
        emit subtractCurrentBytesToIndex(info.folder, bytesChunked);
        if (info.currentPosition < bytes) {
            info.currentPosition = pos;
            info.currentlyProcessing = true;
            enqueueDocumentInternal(info, true /*prepend*/);
            return scheduleNext(folder_id, countForFolder + 1);
        }
    }
    QSqlDatabase::database().commit();
    return scheduleNext(folder_id, countForFolder);
}

void Database::scanDocuments(int folder_id, const QString &folder_path)
{
#if defined(DEBUG)
    qDebug() << "scanning folder for documents" << folder_path;
#endif

    static const QList<QString> extensions { "txt", "pdf", "md", "rst" };

    QDir dir(folder_path);
    Q_ASSERT(dir.exists());
    Q_ASSERT(dir.isReadable());
    QDirIterator it(folder_path, QDir::Readable | QDir::Files, QDirIterator::Subdirectories);
    QVector<DocumentInfo> infos;
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        if (fileInfo.isDir()) {
            addFolderToWatch(fileInfo.canonicalFilePath());
            continue;
        }

        if (!extensions.contains(fileInfo.suffix()))
            continue;

        DocumentInfo info;
        info.folder = folder_id;
        info.doc = fileInfo;
        infos.append(info);
    }

    if (!infos.isEmpty()) {
        emit updateIndexing(folder_id, true);
        enqueueDocuments(folder_id, infos);
    }
}

void Database::start()
{
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &Database::directoryChanged);
    connect(m_embLLM, &EmbeddingLLM::embeddingsGenerated, this, &Database::handleEmbeddingsGenerated);
    connect(m_embLLM, &EmbeddingLLM::errorGenerated, this, &Database::handleErrorGenerated);
    connect(this, &Database::docsToScanChanged, this, &Database::scanQueue);
    if (!QSqlDatabase::drivers().contains("QSQLITE")) {
        qWarning() << "ERROR: missing sqllite driver";
    } else {
        QSqlError err = initDb();
        if (err.type() != QSqlError::NoError)
            qWarning() << "ERROR: initializing db" << err.text();
    }

    if (m_embeddings->fileExists() && !m_embeddings->load())
        qWarning() << "ERROR: Could not load embeddings";

    addCurrentFolders();
}

void Database::addCurrentFolders()
{
#if defined(DEBUG)
    qDebug() << "addCurrentFolders";
#endif

    QSqlQuery q;
    QList<CollectionItem> collections;
    if (!selectAllFromCollections(q, &collections)) {
        qWarning() << "ERROR: Cannot select collections" << q.lastError();
        return;
    }

    emit collectionListUpdated(collections);

    for (const auto &i : collections)
        addFolder(i.collection, i.folder_path);
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

    if (!folders.contains(folder_id)) {
        if (!addCollection(q, collection, folder_id)) {
            qWarning() << "ERROR: Cannot add folder to collection" << collection << path << q.lastError();
            return;
        }

        CollectionItem i;
        i.collection = collection;
        i.folder_path = path;
        i.folder_id = folder_id;
        emit addCollectionItem(i);
    }

    addFolderToWatch(path);
    scanDocuments(folder_id, path);
}

void Database::removeFolder(const QString &collection, const QString &path)
{
#if defined(DEBUG)
    qDebug() << "removeFolder" << path;
#endif

    QSqlQuery q;
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

    removeFolderInternal(collection, folder_id, path);
}

void Database::removeFolderInternal(const QString &collection, int folder_id, const QString &path)
{
    // Determine if the folder is used by more than one collection
    QSqlQuery q;
    QList<QString> collections;
    if (!selectCollectionsFromFolder(q, folder_id, &collections)) {
        qWarning() << "ERROR: Cannot select collections from folder" << folder_id << q.lastError();
        return;
    }

    // Remove it from the collections
    if (!removeCollection(q, collection, folder_id)) {
        qWarning() << "ERROR: Cannot remove collection" << collection << folder_id << q.lastError();
        return;
    }

    // If the folder is associated with more than one collection, then return
    if (collections.count() > 1)
        return;

    // First remove all upcoming jobs associated with this folder
    removeFolderFromDocumentQueue(folder_id);

    // Get a list of all documents associated with folder
    QList<int> documentIds;
    if (!selectDocuments(q, folder_id, &documentIds)) {
        qWarning() << "ERROR: Cannot select documents" << folder_id << q.lastError();
        return;
    }

    // Remove all chunks and documents associated with this folder
    for (int document_id : documentIds) {
        removeEmbeddingsByDocumentId(document_id);
        if (!removeChunksByDocumentId(q, document_id)) {
            qWarning() << "ERROR: Cannot remove chunks of document_id" << document_id << q.lastError();
            return;
        }

        if (!removeDocument(q, document_id)) {
            qWarning() << "ERROR: Cannot remove document_id" << document_id << q.lastError();
            return;
        }
    }

    if (!removeFolderFromDB(q, folder_id)) {
        qWarning() << "ERROR: Cannot remove folder_id" << folder_id << q.lastError();
        return;
    }

    emit removeFolderById(folder_id);

    removeFolderFromWatch(path);
}

bool Database::addFolderToWatch(const QString &path)
{
#if defined(DEBUG)
    qDebug() << "addFolderToWatch" << path;
#endif
    return m_watcher->addPath(path);
}

bool Database::removeFolderFromWatch(const QString &path)
{
#if defined(DEBUG)
    qDebug() << "removeFolderFromWatch" << path;
#endif
    return m_watcher->removePath(path);
}

void Database::retrieveFromDB(const QList<QString> &collections, const QString &text, int retrievalSize,
    QList<ResultInfo> *results)
{
#if defined(DEBUG)
    qDebug() << "retrieveFromDB" << collections << text << retrievalSize;
#endif

    QSqlQuery q;
    if (m_embeddings->isLoaded()) {
        std::vector<float> result = m_embLLM->generateEmbeddings(text);
        if (result.empty()) {
            qDebug() << "ERROR: generating embeddings returned a null result";
            return;
        }
        std::vector<qint64> embeddings = m_embeddings->search(result, retrievalSize);
        if (!selectChunk(q, collections, embeddings, retrievalSize)) {
            qDebug() << "ERROR: selecting chunks:" << q.lastError().text();
            return;
        }
    } else {
        if (!selectChunk(q, collections, text, retrievalSize)) {
            qDebug() << "ERROR: selecting chunks:" << q.lastError().text();
            return;
        }
    }

    while (q.next()) {
#if defined(DEBUG)
        const int rowid = q.value(0).toInt();
#endif
        const QString chunk_text = q.value(2).toString();
        const QString date = QDateTime::fromMSecsSinceEpoch(q.value(1).toLongLong()).toString("yyyy, MMMM dd");
        const QString file = q.value(3).toString();
        const QString title = q.value(4).toString();
        const QString author = q.value(5).toString();
        const int page = q.value(6).toInt();
        const int from =q.value(7).toInt();
        const int to =q.value(8).toInt();
        ResultInfo info;
        info.file = file;
        info.title = title;
        info.author = author;
        info.date = date;
        info.text = chunk_text;
        info.page = page;
        info.from = from;
        info.to = to;
        results->append(info);
#if defined(DEBUG)
        qDebug() << "retrieve rowid:" << rowid
                 << "chunk_text:" << chunk_text;
#endif
    }
}

void Database::cleanDB()
{
#if defined(DEBUG)
    qDebug() << "cleanDB";
#endif

    // Scan all folders in db to make sure they still exist
    QSqlQuery q;
    QList<CollectionItem> collections;
    if (!selectAllFromCollections(q, &collections)) {
        qWarning() << "ERROR: Cannot select collections" << q.lastError();
        return;
    }

    for (const auto &i : collections) {
        // Find the path for the folder
        QFileInfo info(i.folder_path);
        if (!info.exists() || !info.isReadable()) {
#if defined(DEBUG)
            qDebug() << "clean db removing folder" << i.folder_id << i.folder_path;
#endif
            removeFolderInternal(i.collection, i.folder_id, i.folder_path);
        }
    }

    // Scan all documents in db to make sure they still exist
    if (!q.prepare(SELECT_ALL_DOCUMENTS_SQL)) {
        qWarning() << "ERROR: Cannot prepare sql for select all documents" << q.lastError();
        return;
    }

    if (!q.exec()) {
        qWarning() << "ERROR: Cannot exec sql for select all documents" << q.lastError();
        return;
    }

    while (q.next()) {
        int document_id = q.value(0).toInt();
        QString document_path = q.value(1).toString();
        QFileInfo info(document_path);
        if (info.exists() && info.isReadable())
            continue;

#if defined(DEBUG)
        qDebug() << "clean db removing document" << document_id << document_path;
#endif

        // Remove all chunks and documents that either don't exist or have become unreadable
        QSqlQuery query;
        removeEmbeddingsByDocumentId(document_id);
        if (!removeChunksByDocumentId(query, document_id)) {
            qWarning() << "ERROR: Cannot remove chunks of document_id" << document_id << query.lastError();
        }

        if (!removeDocument(query, document_id)) {
            qWarning() << "ERROR: Cannot remove document_id" << document_id << query.lastError();
        }
    }
}

void Database::changeChunkSize(int chunkSize)
{
    if (chunkSize == m_chunkSize)
        return;

#if defined(DEBUG)
    qDebug() << "changeChunkSize" << chunkSize;
#endif

    m_chunkSize = chunkSize;

    QSqlQuery q;
    // Scan all documents in db to make sure they still exist
    if (!q.prepare(SELECT_ALL_DOCUMENTS_SQL)) {
        qWarning() << "ERROR: Cannot prepare sql for select all documents" << q.lastError();
        return;
    }

    if (!q.exec()) {
        qWarning() << "ERROR: Cannot exec sql for select all documents" << q.lastError();
        return;
    }

    while (q.next()) {
        int document_id = q.value(0).toInt();
        // Remove all chunks and documents to change the chunk size
        QSqlQuery query;
        removeEmbeddingsByDocumentId(document_id);
        if (!removeChunksByDocumentId(query, document_id)) {
            qWarning() << "ERROR: Cannot remove chunks of document_id" << document_id << query.lastError();
        }

        if (!removeDocument(query, document_id)) {
            qWarning() << "ERROR: Cannot remove document_id" << document_id << query.lastError();
        }
    }
    addCurrentFolders();
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

    // Clean the database
    cleanDB();

    // Rescan the documents associated with the folder
    scanDocuments(folder_id, path);
}
