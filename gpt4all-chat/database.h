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

struct CollectionItem {
    QString collection;
    QString folder_path;
    int folder_id = -1;
};
Q_DECLARE_METATYPE(CollectionItem)

class Database : public QObject
{
    Q_OBJECT
public:
    Database();

public Q_SLOTS:
    void scanQueue();
    void scanDocuments(int folder_id, const QString &folder_path);
    void addFolder(const QString &collection, const QString &path);
    void removeFolder(const QString &collection, const QString &path);
    void retrieveFromDB(const QList<QString> &collections, const QString &text);
    void cleanDB();

Q_SIGNALS:
    void docsToScanChanged();
    void retrieveResult(const QList<QString> &result);
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
    void chunkStream(QTextStream &stream, int document_id);
    void handleDocumentErrorAndScheduleNext(const QString &errorMessage,
        int document_id, const QString &document_path, const QSqlError &error);

private:
    QQueue<DocumentInfo> m_docsToScan;
    QList<QString> m_retrieve;
    QThread m_dbThread;
    QFileSystemWatcher *m_watcher;
};

#endif // DATABASE_H
