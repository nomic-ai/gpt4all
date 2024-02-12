#include "localdocs.h"
#include "mysettings.h"

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
    connect(MySettings::globalInstance(), &MySettings::localDocsChunkSizeChanged, this, &LocalDocs::handleChunkSizeChanged);

    // Create the DB with the chunk size from settings
    m_database = new Database(MySettings::globalInstance()->localDocsChunkSize());

    connect(this, &LocalDocs::requestAddFolder, m_database,
        &Database::addFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestRemoveFolder, m_database,
        &Database::removeFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestChunkSizeChange, m_database,
        &Database::changeChunkSize, Qt::QueuedConnection);

    // Connections for modifying the model and keeping it updated with the database
    connect(m_database, &Database::updateInstalled,
        m_localDocsModel, &LocalDocsModel::updateInstalled, Qt::QueuedConnection);
    connect(m_database, &Database::updateIndexing,
        m_localDocsModel, &LocalDocsModel::updateIndexing, Qt::QueuedConnection);
    connect(m_database, &Database::updateError,
        m_localDocsModel, &LocalDocsModel::updateError, Qt::QueuedConnection);
    connect(m_database, &Database::updateCurrentDocsToIndex,
        m_localDocsModel, &LocalDocsModel::updateCurrentDocsToIndex, Qt::QueuedConnection);
    connect(m_database, &Database::updateTotalDocsToIndex,
        m_localDocsModel, &LocalDocsModel::updateTotalDocsToIndex, Qt::QueuedConnection);
    connect(m_database, &Database::subtractCurrentBytesToIndex,
        m_localDocsModel, &LocalDocsModel::subtractCurrentBytesToIndex, Qt::QueuedConnection);
    connect(m_database, &Database::updateCurrentBytesToIndex,
        m_localDocsModel, &LocalDocsModel::updateCurrentBytesToIndex, Qt::QueuedConnection);
    connect(m_database, &Database::updateTotalBytesToIndex,
        m_localDocsModel, &LocalDocsModel::updateTotalBytesToIndex, Qt::QueuedConnection);
    connect(m_database, &Database::updateCurrentEmbeddingsToIndex,
            m_localDocsModel, &LocalDocsModel::updateCurrentEmbeddingsToIndex, Qt::QueuedConnection);
    connect(m_database, &Database::updateTotalEmbeddingsToIndex,
            m_localDocsModel, &LocalDocsModel::updateTotalEmbeddingsToIndex, Qt::QueuedConnection);
    connect(m_database, &Database::addCollectionItem,
        m_localDocsModel, &LocalDocsModel::addCollectionItem, Qt::QueuedConnection);
    connect(m_database, &Database::removeFolderById,
        m_localDocsModel, &LocalDocsModel::removeFolderById, Qt::QueuedConnection);
    connect(m_database, &Database::removeCollectionItem,
        m_localDocsModel, &LocalDocsModel::removeCollectionItem, Qt::QueuedConnection);
    connect(m_database, &Database::collectionListUpdated,
        m_localDocsModel, &LocalDocsModel::collectionListUpdated, Qt::QueuedConnection);

    connect(qApp, &QCoreApplication::aboutToQuit, this, &LocalDocs::aboutToQuit);
}

void LocalDocs::aboutToQuit()
{
    delete m_database;
    m_database = nullptr;
}

void LocalDocs::addFolder(const QString &collection, const QString &path)
{
    const QUrl url(path);
    const QString localPath = url.isLocalFile() ? url.toLocalFile() : path;
    emit requestAddFolder(collection, localPath);
}

void LocalDocs::removeFolder(const QString &collection, const QString &path)
{
    m_localDocsModel->removeCollectionPath(collection, path);
    emit requestRemoveFolder(collection, path);
}

void LocalDocs::handleChunkSizeChanged()
{
    emit requestChunkSizeChange(MySettings::globalInstance()->localDocsChunkSize());
}
