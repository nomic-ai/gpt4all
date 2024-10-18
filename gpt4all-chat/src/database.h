#ifndef DATABASE_H
#define DATABASE_H

#include "embllm.h" // IWYU pragma: keep

#include <QByteArray>
#include <QChar>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QHash>
#include <QLatin1String>
#include <QList>
#include <QObject>
#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QUrl>
#include <QVector>
#include <QtGlobal>

#include <atomic>
#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace Qt::Literals::StringLiterals;

class Database;
class DocumentReader;
class QFileSystemWatcher;
class QSqlQuery;
class QTextStream;
class QTimer;

/* Version 0: GPT4All v2.4.3, full-text search
 * Version 1: GPT4All v2.5.3, embeddings in hsnwlib
 * Version 2: GPT4All v3.0.0, embeddings in sqlite
 * Version 3: GPT4All v3.4.0, hybrid search
 */

// minimum supported version
static const int LOCALDOCS_MIN_VER = 1;

// FIXME: (Adam) The next time we bump the version we should add triggers to manage the fts external
// content table as recommended in the official documentation to keep the fts index in sync
// See: https://www.sqlite.org/fts5.html#external_content_tables

// FIXME: (Adam) The fts virtual table should include the chunk_id explicitly instead of relying upon
// the id of the two tables to be in sync

// current version
static const int LOCALDOCS_VERSION = 3;

struct DocumentInfo
{
    using key_type = std::pair<int, QString>;

    int       folder;
    QFileInfo file;
    bool      currentlyProcessing = false;

    key_type key() const { return {folder, file.canonicalFilePath()}; } // for comparison

    bool isPdf () const { return !file.suffix().compare("pdf"_L1,  Qt::CaseInsensitive); }
    bool isDocx() const { return !file.suffix().compare("docx"_L1, Qt::CaseInsensitive); }
};

struct ResultInfo {
    Q_GADGET
    Q_PROPERTY(QString collection MEMBER collection)
    Q_PROPERTY(QString path MEMBER path)
    Q_PROPERTY(QString file MEMBER file)
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString author MEMBER author)
    Q_PROPERTY(QString date MEMBER date)
    Q_PROPERTY(QString text MEMBER text)
    Q_PROPERTY(int page MEMBER page)
    Q_PROPERTY(int from MEMBER from)
    Q_PROPERTY(int to MEMBER to)
    Q_PROPERTY(QString fileUri READ fileUri STORED false)

public:
    QString collection; // [Required] The name of the collection
    QString path;       // [Required] The full path
    QString file;       // [Required] The name of the file, but not the full path
    QString title;      // [Optional] The title of the document
    QString author;     // [Optional] The author of the document
    QString date;       // [Required] The creation or the last modification date whichever is latest
    QString text;       // [Required] The text actually used in the augmented context
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

    bool operator==(const ResultInfo &other) const {
        return file == other.file &&
               title == other.title &&
               author == other.author &&
               date == other.date &&
               text == other.text &&
               page == other.page &&
               from == other.from &&
               to == other.to;
    }
    bool operator!=(const ResultInfo &other) const {
        return !(*this == other);
    }
};

Q_DECLARE_METATYPE(ResultInfo)

struct CollectionItem {
    // -- Fields persisted to database --

    int collection_id = -1;
    int folder_id = -1;
    QString collection;
    QString folder_path;
    QString embeddingModel;

    // -- Transient fields --

    bool installed = false;
    bool indexing = false;
    bool forceIndexing = false;
    QString error;

    // progress
    int currentDocsToIndex = 0;
    int totalDocsToIndex = 0;
    size_t currentBytesToIndex = 0;
    size_t totalBytesToIndex = 0;
    size_t currentEmbeddingsToIndex = 0;
    size_t totalEmbeddingsToIndex = 0;

    // statistics
    size_t totalDocs = 0;
    size_t totalWords = 0;
    size_t totalTokens = 0;
    QDateTime startUpdate;
    QDateTime lastUpdate;
    QString fileCurrentlyProcessing;
};
Q_DECLARE_METATYPE(CollectionItem)

class ChunkStreamer {
public:
    enum class Status { DOC_COMPLETE, INTERRUPTED, ERROR, BINARY_SEEN };

    explicit ChunkStreamer(Database *database);
    ~ChunkStreamer();

    void setDocument(const DocumentInfo &doc, int documentId, const QString &embeddingModel);
    std::optional<DocumentInfo::key_type> currentDocKey() const;
    void reset();

    Status step();

private:
    Database                              *m_database;
    std::optional<DocumentInfo::key_type>  m_docKey;
    std::unique_ptr<DocumentReader>        m_reader; // may be invalid, always compare key first
    int                                    m_documentId;
    QString                                m_embeddingModel;
    QString                                m_title;
    QString                                m_author;
    QString                                m_subject;
    QString                                m_keywords;

    // working state
    QString                                m_chunk; // has a trailing space for convenience
    int                                    m_nChunkWords = 0;
    int                                    m_page = 0;
};

class Database : public QObject
{
    Q_OBJECT
public:
    Database(int chunkSize, QStringList extensions);
    ~Database() override;

    bool isValid() const { return m_databaseValid; }

public Q_SLOTS:
    void start();
    bool scanQueueInterrupted() const;
    void scanQueueBatch();
    void scanDocuments(int folder_id, const QString &folder_path);
    void forceIndexing(const QString &collection, const QString &embedding_model);
    void forceRebuildFolder(const QString &path);
    bool addFolder(const QString &collection, const QString &path, const QString &embedding_model);
    void removeFolder(const QString &collection, const QString &path);
    void retrieveFromDB(const QList<QString> &collections, const QString &text, int retrievalSize, QList<ResultInfo> *results);
    void changeChunkSize(int chunkSize);
    void changeFileExtensions(const QStringList &extensions);

Q_SIGNALS:
    // Signals for the gui only
    void requestUpdateGuiForCollectionItem(const CollectionItem &item);
    void requestAddGuiCollectionItem(const CollectionItem &item);
    void requestRemoveGuiFolderById(const QString &collection, int folder_id);
    void requestGuiCollectionListUpdated(const QList<CollectionItem> &collectionList);
    void databaseValidChanged();

private Q_SLOTS:
    void directoryChanged(const QString &path);
    void addCurrentFolders();
    void handleEmbeddingsGenerated(const QVector<EmbeddingResult> &embeddings);
    void handleErrorGenerated(const QVector<EmbeddingChunk> &chunks, const QString &error);

private:
    void transaction();
    void commit();
    void rollback();

    bool addChunk(QSqlQuery &q, int document_id, const QString &chunk_text, const QString &file,
                  const QString &title, const QString &author, const QString &subject, const QString &keywords,
                  int page, int from, int to, int words, int *chunk_id);
    bool refreshDocumentIdCache(QSqlQuery &q);
    bool removeChunksByDocumentId(QSqlQuery &q, int document_id);
    bool sqlRemoveDocsByFolderPath(QSqlQuery &q, const QString &path);
    bool hasContent();
    // not found -> 0, , exists and has content -> 1, error -> -1
    int openDatabase(const QString &modelPath, bool create = true, int ver = LOCALDOCS_VERSION);
    bool openLatestDb(const QString &modelPath, QList<CollectionItem> &oldCollections);
    bool initDb(const QString &modelPath, const QList<CollectionItem> &oldCollections);
    int checkAndAddFolderToDB(const QString &path);
    bool removeFolderInternal(const QString &collection, int folder_id, const QString &path);
    size_t chunkStream(QTextStream &stream, int folder_id, int document_id, const QString &embedding_model,
        const QString &file, const QString &title, const QString &author, const QString &subject,
        const QString &keywords, int page, int maxChunks = -1);
    void appendChunk(const EmbeddingChunk &chunk);
    void sendChunkList();
    void updateFolderToIndex(int folder_id, size_t countForFolder, bool sendChunks = true);
    size_t countOfDocuments(int folder_id) const;
    size_t countOfBytes(int folder_id) const;
    DocumentInfo dequeueDocument();
    void removeFolderFromDocumentQueue(int folder_id);
    void enqueueDocumentInternal(DocumentInfo &&info, bool prepend = false);
    void enqueueDocuments(int folder_id, std::list<DocumentInfo> &&infos);
    void scanQueue();
    bool ftsIntegrityCheck();
    bool cleanDB();
    void addFolderToWatch(const QString &path);
    void removeFolderFromWatch(const QString &path);
    static QList<int> searchEmbeddingsHelper(const std::vector<float> &query, QSqlQuery &q, int nNeighbors);
    QList<int> searchEmbeddings(const std::vector<float> &query, const QList<QString> &collections,
        int nNeighbors);
    struct BM25Query {
        QString input;
        QString query;
        bool isExact = false;
        int qlength = 0;
        int ilength = 0;
        int rlength = 0;
    };
    QList<Database::BM25Query> queriesForFTS5(const QString &input);
    QList<int> searchBM25(const QString &query, const QList<QString> &collections, BM25Query &bm25q, int k);
    QList<int> scoreChunks(const std::vector<float> &query, const QList<int> &chunks);
    float computeBM25Weight(const BM25Query &bm25q);
    QList<int> reciprocalRankFusion(const std::vector<float> &query, const QList<int> &embeddingResults,
        const QList<int> &bm25Results, const BM25Query &bm25q, int k);
    QList<int> searchDatabase(const QString &query, const QList<QString> &collections, int k);

    void setStartUpdateTime(CollectionItem &item);
    void setLastUpdateTime(CollectionItem &item);

    CollectionItem guiCollectionItem(int folder_id) const;
    void updateGuiForCollectionItem(const CollectionItem &item);
    void addGuiCollectionItem(const CollectionItem &item);
    void removeGuiFolderById(const QString &collection, int folder_id);
    void guiCollectionListUpdated(const QList<CollectionItem> &collectionList);
    void scheduleUncompletedEmbeddings();
    void updateCollectionStatistics();

private:
    QSqlDatabase m_db;
    int m_chunkSize;
    QStringList m_scannedFileExtensions;
    QTimer *m_scanIntervalTimer;
    QElapsedTimer m_scanDurationTimer;
    std::map<int, std::list<DocumentInfo>> m_docsToScan;
    QList<ResultInfo> m_retrieve;
    QThread m_dbThread;
    QFileSystemWatcher *m_watcher;
    QSet<QString> m_watchedPaths;
    EmbeddingLLM *m_embLLM;
    QVector<EmbeddingChunk> m_chunkList;
    QHash<int, CollectionItem> m_collectionMap; // used only for tracking indexing/embedding progress
    std::atomic<bool> m_databaseValid;
    ChunkStreamer m_chunkStreamer;
    QSet<int> m_documentIdCache; // cached list of documents with chunks for fast lookup

    friend class ChunkStreamer;
};

#endif // DATABASE_H
