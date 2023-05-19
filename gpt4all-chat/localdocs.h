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

Q_SIGNALS:
    void docsToScanChanged();
    void retrieveResult(const QList<QString> &result);

private Q_SLOTS:
    void start();
    void directoryChanged(const QString &path);
    bool addFolderToWatch(const QString &path);
    bool removeFolderFromWatch(const QString &path);

private:
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

public:
    static LocalDocs *globalInstance();

    void addFolder(const QString &collection, const QString &path);
    void removeFolder(const QString &collection, const QString &path);

    QList<QString> result() const { return m_retrieveResult; }
    void requestRetrieve(const QList<QString> &collections, const QString &text);

Q_SIGNALS:
    void requestAddFolder(const QString &collection, const QString &path);
    void requestRemoveFolder(const QString &collection, const QString &path);
    void requestRetrieveFromDB(const QList<QString> &collections, const QString &text);
    void receivedResult();

private Q_SLOTS:
    void retrieveResult(const QList<QString> &result);

private:
    Database *m_database;
    QList<QString> m_retrieveResult;

private:
    explicit LocalDocs();
    ~LocalDocs() {}
    friend class MyLocalDocs;
};

#endif // LOCALDOCS_H
