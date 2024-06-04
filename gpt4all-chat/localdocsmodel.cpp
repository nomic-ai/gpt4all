#include "localdocsmodel.h"

#include "localdocs.h"
#include "network.h"

#include <QMap>
#include <QtGlobal>

#include <utility>

LocalDocsCollectionsModel::LocalDocsCollectionsModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(LocalDocs::globalInstance()->localDocsModel());
}

bool LocalDocsCollectionsModel::filterAcceptsRow(int sourceRow,
                                       const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    const QString collection = sourceModel()->data(index, LocalDocsModel::CollectionRole).toString();
    return m_collections.contains(collection);
}

void LocalDocsCollectionsModel::setCollections(const QList<QString> &collections)
{
    m_collections = collections;
    invalidateFilter();
}

LocalDocsModel::LocalDocsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LocalDocsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_collectionList.size();
}

QVariant LocalDocsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_collectionList.size())
        return QVariant();

    const CollectionItem item = m_collectionList.at(index.row());
    switch (role) {
        case CollectionRole:
            return item.collection;
        case FolderPathRole:
            return item.folder_path;
        case InstalledRole:
            return item.installed;
        case IndexingRole:
            return item.indexing;
        case ErrorRole:
            return item.error;
        case CurrentDocsToIndexRole:
            return item.currentDocsToIndex;
        case TotalDocsToIndexRole:
            return item.totalDocsToIndex;
        case CurrentBytesToIndexRole:
            return quint64(item.currentBytesToIndex);
        case TotalBytesToIndexRole:
            return quint64(item.totalBytesToIndex);
        case CurrentEmbeddingsToIndexRole:
            return quint64(item.currentEmbeddingsToIndex);
        case TotalEmbeddingsToIndexRole:
            return quint64(item.totalEmbeddingsToIndex);
    }

    return QVariant();
}

QHash<int, QByteArray> LocalDocsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CollectionRole] = "collection";
    roles[FolderPathRole] = "folder_path";
    roles[InstalledRole] = "installed";
    roles[IndexingRole] = "indexing";
    roles[ErrorRole] = "error";
    roles[CurrentDocsToIndexRole] = "currentDocsToIndex";
    roles[TotalDocsToIndexRole] = "totalDocsToIndex";
    roles[CurrentBytesToIndexRole] = "currentBytesToIndex";
    roles[TotalBytesToIndexRole] = "totalBytesToIndex";
    roles[CurrentEmbeddingsToIndexRole] = "currentEmbeddingsToIndex";
    roles[TotalEmbeddingsToIndexRole] = "totalEmbeddingsToIndex";
    return roles;
}

template<typename T>
void LocalDocsModel::updateField(int folder_id, T value,
    const std::function<void(CollectionItem&, T)>& updater,
    const QVector<int>& roles)
{
    for (int i = 0; i < m_collectionList.size(); ++i) {
        if (m_collectionList.at(i).folder_id != folder_id)
            continue;

        updater(m_collectionList[i], value);
        emit dataChanged(this->index(i), this->index(i), roles);
    }
}

void LocalDocsModel::updateInstalled(int folder_id, bool b)
{
    updateField<bool>(folder_id, b,
        [](CollectionItem& item, bool val) { item.installed = val; }, {InstalledRole});
}

void LocalDocsModel::updateIndexing(int folder_id, bool b)
{
    updateField<bool>(folder_id, b,
        [](CollectionItem& item, bool val) { item.indexing = val; }, {IndexingRole});
}

void LocalDocsModel::updateError(int folder_id, const QString &error)
{
    updateField<QString>(folder_id, error,
        [](CollectionItem& item, QString val) { item.error = val; }, {ErrorRole});
}

void LocalDocsModel::updateCurrentDocsToIndex(int folder_id, size_t currentDocsToIndex)
{
    updateField<size_t>(folder_id, currentDocsToIndex,
        [](CollectionItem& item, size_t val) { item.currentDocsToIndex = val; }, {CurrentDocsToIndexRole});
}

void LocalDocsModel::updateTotalDocsToIndex(int folder_id, size_t totalDocsToIndex)
{
    updateField<size_t>(folder_id, totalDocsToIndex,
        [](CollectionItem& item, size_t val) { item.totalDocsToIndex = val; }, {TotalDocsToIndexRole});
}

void LocalDocsModel::subtractCurrentBytesToIndex(int folder_id, size_t subtractedBytes)
{
    updateField<size_t>(folder_id, subtractedBytes,
        [](CollectionItem& item, size_t val) { item.currentBytesToIndex -= val; }, {CurrentBytesToIndexRole});
}

void LocalDocsModel::updateCurrentBytesToIndex(int folder_id, size_t currentBytesToIndex)
{
    updateField<size_t>(folder_id, currentBytesToIndex,
        [](CollectionItem& item, size_t val) { item.currentBytesToIndex = val; }, {CurrentBytesToIndexRole});
}

void LocalDocsModel::updateTotalBytesToIndex(int folder_id, size_t totalBytesToIndex)
{
    updateField<size_t>(folder_id, totalBytesToIndex,
        [](CollectionItem& item, size_t val) { item.totalBytesToIndex = val; }, {TotalBytesToIndexRole});
}

void LocalDocsModel::updateCurrentEmbeddingsToIndex(int folder_id, size_t currentEmbeddingsToIndex)
{
    updateField<size_t>(folder_id, currentEmbeddingsToIndex,
        [](CollectionItem& item, size_t val) { item.currentEmbeddingsToIndex += val; }, {CurrentEmbeddingsToIndexRole});
}

void LocalDocsModel::updateTotalEmbeddingsToIndex(int folder_id, size_t totalEmbeddingsToIndex)
{
    updateField<size_t>(folder_id, totalEmbeddingsToIndex,
        [](CollectionItem& item, size_t val) { item.totalEmbeddingsToIndex += val; }, {TotalEmbeddingsToIndexRole});
}

void LocalDocsModel::addCollectionItem(const CollectionItem &item, bool fromDb)
{
    beginInsertRows(QModelIndex(), m_collectionList.size(), m_collectionList.size());
    m_collectionList.append(item);
    endInsertRows();

    if (!fromDb) {
        Network::globalInstance()->trackEvent("doc_collection_add", {
            {"collection_count", m_collectionList.count()},
        });
    }
}

void LocalDocsModel::removeCollectionIf(std::function<bool(CollectionItem)> const &predicate) {
    for (int i = 0; i < m_collectionList.size();) {
        if (predicate(m_collectionList.at(i))) {
            beginRemoveRows(QModelIndex(), i, i);
            m_collectionList.removeAt(i);
            endRemoveRows();

            Network::globalInstance()->trackEvent("doc_collection_remove", {
                {"collection_count", m_collectionList.count()},
            });
        } else {
            ++i;
        }
    }
}

void LocalDocsModel::removeFolderById(int folder_id)
{
    removeCollectionIf([folder_id](const auto &c) { return c.folder_id == folder_id; });
}

void LocalDocsModel::removeCollectionPath(const QString &name, const QString &path)
{
    removeCollectionIf([&name, &path](const auto &c) { return c.collection == name && c.folder_path == path; });
}

void LocalDocsModel::collectionListUpdated(const QList<CollectionItem> &collectionList)
{
    beginResetModel();
    m_collectionList = collectionList;
    endResetModel();
}
