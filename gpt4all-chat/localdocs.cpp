#include "localdocs.h"

#include "database.h"
#include "embllm.h"
#include "mysettings.h"

#include <QCoreApplication>
#include <QGlobalStatic>
#include <QGuiApplication>
#include <QUrl>
#include <Qt>

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
    connect(MySettings::globalInstance(), &MySettings::localDocsFileExtensionsChanged, this, &LocalDocs::handleFileExtensionsChanged);

    // Create the DB with the chunk size from settings
    m_database = new Database(MySettings::globalInstance()->localDocsChunkSize(),
                              MySettings::globalInstance()->localDocsFileExtensions());

    connect(this, &LocalDocs::requestStart, m_database,
        &Database::start, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestForceIndexing, m_database,
        &Database::forceIndexing, Qt::QueuedConnection);
    connect(this, &LocalDocs::forceRebuildFolder, m_database,
        &Database::forceRebuildFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestAddFolder, m_database,
        &Database::addFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestRemoveFolder, m_database,
        &Database::removeFolder, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestChunkSizeChange, m_database,
        &Database::changeChunkSize, Qt::QueuedConnection);
    connect(this, &LocalDocs::requestFileExtensionsChange, m_database,
        &Database::changeFileExtensions, Qt::QueuedConnection);
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

    connect(qGuiApp, &QCoreApplication::aboutToQuit, this, &LocalDocs::aboutToQuit);
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

    const QString embedding_model = EmbeddingLLM::model();
    if (embedding_model.isEmpty()) {
        qWarning() << "ERROR: We have no embedding model";
        return;
    }

    emit requestAddFolder(collection, localPath, embedding_model);
}

void LocalDocs::removeFolder(const QString &collection, const QString &path)
{
    emit requestRemoveFolder(collection, path);
}

void LocalDocs::forceIndexing(const QString &collection)
{
    const QString embedding_model = EmbeddingLLM::model();
    if (embedding_model.isEmpty()) {
        qWarning() << "ERROR: We have no embedding model";
        return;
    }

    emit requestForceIndexing(collection, embedding_model);
}

void LocalDocs::handleChunkSizeChanged()
{
    emit requestChunkSizeChange(MySettings::globalInstance()->localDocsChunkSize());
}

void LocalDocs::handleFileExtensionsChanged()
{
    emit requestFileExtensionsChange(MySettings::globalInstance()->localDocsFileExtensions());
}
