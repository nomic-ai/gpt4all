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

void LocalDocs::handleChunkSizeChanged()
{
    emit requestChunkSizeChange(MySettings::globalInstance()->localDocsChunkSize());
}
