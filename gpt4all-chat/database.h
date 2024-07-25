#ifndef DATABASE_H
#define DATABASE_H

#include "embllm.h" // IWYU pragma: keep
#include "sourceexcerpt.h"

#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QLatin1String>
#include <QList>
#include <QMap>
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QUrl>
#include <QVector>

#include <cstddef>

using namespace Qt::Literals::StringLiterals;

class QFileSystemWatcher;
class QSqlError;
class QTextStream;
class QTimer;

/* Version 0: GPT4All v2.4.3, full-text search
 * Version 1: GPT4All v2.5.3, embeddings in hsnwlib
 * Version 2: GPT4All v3.0.0, embeddings in sqlite */

// minimum supported version
static const int LOCALDOCS_MIN_VER = 1;
// current version
static const int LOCALDOCS_VERSION = 2;

struct DocumentInfo
{
    int folder;
    QFileInfo doc;
    int currentPage = 0;
    size_t currentPosition = 0;
    bool currentlyProcessing = false;
    bool isPdf() const {
        return doc.suffix().compare(u"pdf"_s, Qt::CaseInsensitive) == 0;
    }
};

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

class Database : public QObject
{
    Q_OBJECT
public:
    Database(int chunkSize, QStringList extensions);
    ~Database() override;

    bool isValid() const { return m_databaseValid; }

public Q_SLOTS:
    void start();
    void scanQueueBatch();
    void scanDocuments(int folder_id, const QString &folder_path);
    void forceIndexing(const QString &collection, const QString &embedding_model);
    void forceRebuildFolder(const QString &path);
    bool addFolder(const QString &collection, const QString &path, const QString &embedding_model);
    void removeFolder(const QString &collection, const QString &path);
    void retrieveFromDB(const QList<QString> &collections, const QString &text, int retrievalSize, QList<SourceExcerpt> *results);
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
    void handleDocumentError(const QString &errorMessage,
        int document_id, const QString &document_path, const QSqlError &error);
    size_t countOfDocuments(int folder_id) const;
    size_t countOfBytes(int folder_id) const;
    DocumentInfo dequeueDocument();
    void removeFolderFromDocumentQueue(int folder_id);
    void enqueueDocumentInternal(const DocumentInfo &info, bool prepend = false);
    void enqueueDocuments(int folder_id, const QVector<DocumentInfo> &infos);
    void scanQueue();
    bool cleanDB();
    void addFolderToWatch(const QString &path);
    void removeFolderFromWatch(const QString &path);
    QList<int> searchEmbeddings(const std::vector<float> &query, const QList<QString> &collections, int nNeighbors);

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
    QTimer *m_scanTimer;
    QMap<int, QQueue<DocumentInfo>> m_docsToScan;
    QList<SourceExcerpt> m_retrieve;
    QThread m_dbThread;
    QFileSystemWatcher *m_watcher;
    QSet<QString> m_watchedPaths;
    EmbeddingLLM *m_embLLM;
    QVector<EmbeddingChunk> m_chunkList;
    QHash<int, CollectionItem> m_collectionMap; // used only for tracking indexing/embedding progress
    std::atomic<bool> m_databaseValid;
};

#endif // DATABASE_H
