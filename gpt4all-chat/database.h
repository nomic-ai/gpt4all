#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QtSql>
#include <QQueue>
#include <QFileInfo>
#include <QThread>
#include <QFileSystemWatcher>

struct DocumentInfo
{
    int folder;
    QFileInfo doc;
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
    bool installed = false;
};
Q_DECLARE_METATYPE(CollectionItem)

class Database : public QObject
{
    Q_OBJECT
public:
    Database(int chunkSize);

public Q_SLOTS:
    void scanQueue();
    void scanDocuments(int folder_id, const QString &folder_path);
    void addFolder(const QString &collection, const QString &path);
    void removeFolder(const QString &collection, const QString &path);
    void retrieveFromDB(const QList<QString> &collections, const QString &text, int retrievalSize, QList<ResultInfo> *results);
    void cleanDB();
    void changeChunkSize(int chunkSize);

Q_SIGNALS:
    void docsToScanChanged();
    void collectionListUpdated(const QList<CollectionItem> &collectionList);

private Q_SLOTS:
    void start();
    void directoryChanged(const QString &path);
    bool addFolderToWatch(const QString &path);
    bool removeFolderFromWatch(const QString &path);
    void addCurrentFolders();
    void updateCollectionList();

private:
    void removeFolderInternal(const QString &collection, int folder_id, const QString &path);
    void chunkStream(QTextStream &stream, int document_id, const QString &file,
        const QString &title, const QString &author, const QString &subject, const QString &keywords, int page);
    void handleDocumentErrorAndScheduleNext(const QString &errorMessage,
        int document_id, const QString &document_path, const QSqlError &error);

private:
    int m_chunkSize;
    QQueue<DocumentInfo> m_docsToScan;
    QList<ResultInfo> m_retrieve;
    QThread m_dbThread;
    QFileSystemWatcher *m_watcher;
};

#endif // DATABASE_H
