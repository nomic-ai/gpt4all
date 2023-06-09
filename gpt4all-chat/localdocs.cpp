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
    , m_database(nullptr)
{
    QSettings settings;
    settings.sync();
    m_chunkSize = settings.value("localdocs/chunkSize", 256).toInt();
    m_retrievalSize = settings.value("localdocs/retrievalSize", 3).toInt();

    // Create the DB with the chunk size from settings
    m_database = new Database(m_chunkSize);

    connect(this, &LocalDocs::requestAddFolder, m_database,
        &Database::addFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestRemoveFolder, m_database,
        &Database::removeFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestChunkSizeChange, m_database,
        &Database::changeChunkSize, Qt::QueuedConnection);
    connect(m_database, &Database::collectionListUpdated,
        m_localDocsModel, &LocalDocsModel::handleCollectionListUpdated, Qt::QueuedConnection);
}

void LocalDocs::addFolder(const QString &collection, const QString &path)
{
    const QUrl url(path);
    const QString localPath = url.isLocalFile() ? url.toLocalFile() : path;
    // Add a placeholder collection that is not installed yet
    CollectionItem i;
    i.collection = collection;
    i.folder_path = localPath;
    m_localDocsModel->addCollectionItem(i);
    emit requestAddFolder(collection, localPath);
}

void LocalDocs::removeFolder(const QString &collection, const QString &path)
{
    emit requestRemoveFolder(collection, path);
}

int LocalDocs::chunkSize() const
{
    return m_chunkSize;
}

void LocalDocs::setChunkSize(int chunkSize)
{
    if (m_chunkSize == chunkSize)
        return;

    m_chunkSize = chunkSize;
    emit chunkSizeChanged();
    emit requestChunkSizeChange(chunkSize);
}

int LocalDocs::retrievalSize() const
{
    return m_retrievalSize;
}

void LocalDocs::setRetrievalSize(int retrievalSize)
{
    if (m_retrievalSize == retrievalSize)
        return;

    m_retrievalSize = retrievalSize;
    emit retrievalSizeChanged();
}
