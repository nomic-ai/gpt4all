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

    connect(this, &LocalDocs::requestStart, m_database,
        &Database::start, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestForceIndexing, m_database,
        &Database::forceIndexing, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestAddFolder, m_database,
        &Database::addFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestRemoveFolder, m_database,
        &Database::removeFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestChunkSizeChange, m_database,
        &Database::changeChunkSize, Qt::QueuedConnection);
    connect(m_database, &Database::databaseValidChanged,
        this, &LocalDocs::databaseValidChanged, Qt::QueuedConnection);

    // Connections for modifying the model and keeping it updated with the database
    connect(m_database, &Database::requestUpdateGuiForCollectionItem,
        m_localDocsModel, &LocalDocsModel::updateCollectionItem, Qt::QueuedConnection);
    connect(m_database, &Database::requestAddGuiCollectionItem,
        m_localDocsModel, &LocalDocsModel::addCollectionItem, Qt::QueuedConnection);
    connect(m_database, &Database::requestRemoveGuiFolderById,
        m_localDocsModel, &LocalDocsModel::removeFolderById, Qt::QueuedConnection);
    connect(m_database, &Database::requestGuiCollectionListUpdated,
        m_localDocsModel, &LocalDocsModel::collectionListUpdated, Qt::QueuedConnection);

    connect(qApp, &QCoreApplication::aboutToQuit, this, &LocalDocs::aboutToQuit);
}

void LocalDocs::aboutToQuit()
{
    delete m_database;
    m_database = nullptr;
}

void LocalDocs::forceIndexing(const QString &collection)
{
    emit requestForceIndexing(collection);
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
