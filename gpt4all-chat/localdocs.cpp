#include "localdocs.h"

class MyLocalDocs: public LocalDocs { };
Q_GLOBAL_STATIC(MyLocalDocs, localDocsInstance)
LocalDocs *LocalDocs::globalInstance()
{
    return localDocsInstance();
}

LocalDocs::LocalDocs()
    : QObject(nullptr)
    , m_localDocsModel(new LocalDocsModel(this))
    , m_database(new Database)
{
    connect(this, &LocalDocs::requestAddFolder, m_database,
        &Database::addFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestRemoveFolder, m_database,
        &Database::removeFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestRetrieveFromDB, m_database,
        &Database::retrieveFromDB, Qt::QueuedConnection);
    connect(m_database, &Database::retrieveResult, this,
        &LocalDocs::handleRetrieveResult, Qt::QueuedConnection);
    connect(m_database, &Database::collectionListUpdated,
        m_localDocsModel, &LocalDocsModel::handleCollectionListUpdated, Qt::QueuedConnection);
}

void LocalDocs::addFolder(const QString &collection, const QString &path)
{
    const QUrl url(path);
    if (url.isLocalFile()) {
        emit requestAddFolder(collection, url.toLocalFile());
    } else {
        emit requestAddFolder(collection, path);
    }
}

void LocalDocs::removeFolder(const QString &collection, const QString &path)
{
    emit requestRemoveFolder(collection, path);
}

void LocalDocs::requestRetrieve(const QList<QString> &collections, const QString &text)
{
    m_retrieveResult = QList<QString>();
    emit requestRetrieveFromDB(collections, text);
}

void LocalDocs::handleRetrieveResult(const QList<QString> &result)
{
    m_retrieveResult = result;
    emit receivedResult();
}
