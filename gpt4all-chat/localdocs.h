#ifndef LOCALDOCS_H
#define LOCALDOCS_H

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

struct CollectionInfo {
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
public:
    QString name;
    QList<QString> folders;
};
Q_DECLARE_METATYPE(CollectionInfo)

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
    void collectionListUpdated(const QList<CollectionInfo> &collectionList);

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

class LocalDocs : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<CollectionInfo> collectionList READ collectionList NOTIFY collectionListChanged)

public:
    static LocalDocs *globalInstance();

    QList<CollectionInfo> collectionList() const { return m_collectionList; }

    void addFolder(const QString &collection, const QString &path);
    void removeFolder(const QString &collection, const QString &path);

    QList<QString> result() const { return m_retrieveResult; }
    void requestRetrieve(const QList<QString> &collections, const QString &text);

Q_SIGNALS:
    void requestAddFolder(const QString &collection, const QString &path);
    void requestRemoveFolder(const QString &collection, const QString &path);
    void requestRetrieveFromDB(const QList<QString> &collections, const QString &text);
    void receivedResult();
    void collectionListChanged();

private Q_SLOTS:
    void handleRetrieveResult(const QList<QString> &result);
    void handleCollectionListUpdated(const QList<CollectionInfo> &collectionList);

private:
    Database *m_database;
    QList<QString> m_retrieveResult;
    QList<CollectionInfo> m_collectionList;

private:
    explicit LocalDocs();
    ~LocalDocs() {}
    friend class MyLocalDocs;
};

#endif // LOCALDOCS_H
