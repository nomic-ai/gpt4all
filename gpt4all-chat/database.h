#ifndef DATABASE_H
#define DATABASE_H

#include "embllm.h" // IWYU pragma: keep

#include <QElapsedTimer>
#include <QFileInfo>
#include <QLatin1String>
#include <QList>
#include <QMap>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QThread>
#include <QVector>
#include <QtGlobal>
#include <QtSql>

#include <cstddef>

class EmbeddingLLM;
class Embeddings;
class QFileSystemWatcher;
class QSqlError;
class QTextStream;
class QTimer;

struct DocumentInfo
{
    int folder;
    QFileInfo doc;
    int currentPage = 0;
    size_t currentPosition = 0;
    bool currentlyProcessing = false;
    bool isPdf() const {
        return doc.suffix() == QLatin1String("pdf");
    }
};

struct ResultInfo {
    QString file;   // [Required] The name of the file, but not the full path
    QString title;  // [Optional] The title of the document
    QString author; // [Optional] The author of the document
    QString date;   // [Required] The creation or the last modification date whichever is latest
    QString text;   // [Required] The text actually used in the augmented context
    int page = -1;  // [Optional] The page where the text was found
    int from = -1;  // [Optional] The line number where the text begins
    int to = -1;    // [Optional] The line number where the text ends
};

struct CollectionItem {
    QString collection;
    QString folder_path;
    int folder_id = -1;
    bool installed = false; // not database
    bool indexing = false; // not database
    bool forceIndexing = false;
    QString error; // not database

    // progress
    int currentDocsToIndex = 0; // not database
    int totalDocsToIndex = 0; // not database
    size_t currentBytesToIndex = 0; // not database
    size_t totalBytesToIndex = 0; // not database
    size_t currentEmbeddingsToIndex = 0; // not database
    size_t totalEmbeddingsToIndex = 0; // not database

    // statistics
    size_t totalDocs = 0;
    size_t totalWords = 0;
    size_t totalTokens = 0;
    QDateTime lastUpdate;
    QString fileCurrentlyProcessing;
    QString embeddingModel;
};
Q_DECLARE_METATYPE(CollectionItem)

class Database : public QObject
{
    Q_OBJECT
public:
    Database(int chunkSize);
    virtual ~Database();

    bool isValid() const { return m_databaseValid; }

public Q_SLOTS:
    void start();
    void scanQueueBatch();
    void scanDocuments(int folder_id, const QString &folder_path);
    void forceIndexing(const QString &collection);
    void addFolder(const QString &collection, const QString &path);
    void removeFolder(const QString &collection, const QString &path);
    void retrieveFromDB(const QList<QString> &collections, const QString &text, int retrievalSize, QList<ResultInfo> *results);
    void cleanDB();
    void changeChunkSize(int chunkSize);

Q_SIGNALS:
    // Signals for the gui only
    void requestUpdateGuiForCollectionItem(const CollectionItem &item);
    void requestAddGuiCollectionItem(const CollectionItem &item);
    void requestRemoveGuiFolderById(int folder_id);
    void requestGuiCollectionListUpdated(const QList<CollectionItem> &collectionList);
    void databaseValidChanged();

private Q_SLOTS:
    void directoryChanged(const QString &path);
    bool addFolderToWatch(const QString &path);
    bool removeFolderFromWatch(const QString &path);
    void addCurrentFolders();
    void handleEmbeddingsGenerated(const QVector<EmbeddingResult> &embeddings);
    void handleErrorGenerated(int folder_id, const QString &error);

private:
    static void transaction();
    static void commit();
    static void rollback();

    bool initDb();
    bool addForcedCollection(const CollectionItem &collection);
    void removeFolderInternal(const QString &collection, int folder_id, const QString &path);
    size_t chunkStream(QTextStream &stream, int folder_id, int document_id, const QString &file,
        const QString &title, const QString &author, const QString &subject, const QString &keywords, int page,
        int maxChunks = -1);
    void appendChunk(const EmbeddingChunk &chunk);
    void sendChunkList();
    bool getChunksByDocumentId(int document_id, QList<int> &chunkIds);
    void scheduleNext(int folder_id, size_t countForFolder);
    void handleDocumentError(const QString &errorMessage,
        int document_id, const QString &document_path, const QSqlError &error);
    size_t countOfDocuments(int folder_id) const;
    size_t countOfBytes(int folder_id) const;
    DocumentInfo dequeueDocument();
    void removeFolderFromDocumentQueue(int folder_id);
    void enqueueDocumentInternal(const DocumentInfo &info, bool prepend = false);
    void enqueueDocuments(int folder_id, const QVector<DocumentInfo> &infos);
    bool scanQueue(QList<int> &chunksToRemove);

    CollectionItem guiCollectionItem(int folder_id) const;
    void updateGuiForCollectionItem(const CollectionItem &item);
    void addGuiCollectionItem(const CollectionItem &item);
    void removeGuiFolderById(int folder_id);
    void guiCollectionListUpdated(const QList<CollectionItem> &collectionList);
    void scheduleUncompletedEmbeddings(int folder_id);
    void updateCollectionStatistics();

private:
    int m_chunkSize;
    QTimer *m_scanTimer;
    QMap<int, QQueue<DocumentInfo>> m_docsToScan;
    QList<ResultInfo> m_retrieve;
    QThread m_dbThread;
    QFileSystemWatcher *m_watcher;
    EmbeddingLLM *m_embLLM;
    Embeddings *m_embeddings;
    QVector<EmbeddingChunk> m_chunkList;
    QHash<int, CollectionItem> m_collectionMap; // used only for tracking indexing/embedding progress
    std::atomic<bool> m_databaseValid;
};

#endif // DATABASE_H
