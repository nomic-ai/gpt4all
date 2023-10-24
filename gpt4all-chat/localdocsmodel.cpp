#include "localdocsmodel.h"

#include "localdocs.h"

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
        case CurrentDocsToIndexRole:
            return item.currentDocsToIndex;
        case TotalDocsToIndexRole:
            return item.totalDocsToIndex;
        case CurrentBytesToIndexRole:
            return quint64(item.currentBytesToIndex);
        case TotalBytesToIndexRole:
            return quint64(item.totalBytesToIndex);
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
    roles[CurrentDocsToIndexRole] = "currentDocsToIndex";
    roles[TotalDocsToIndexRole] = "totalDocsToIndex";
    roles[CurrentBytesToIndexRole] = "currentBytesToIndex";
    roles[TotalBytesToIndexRole] = "totalBytesToIndex";
    return roles;
}

void LocalDocsModel::updateInstalled(int folder_id, bool b)
{
    for (int i = 0; i < m_collectionList.size(); ++i) {
        if (m_collectionList.at(i).folder_id != folder_id)
            continue;

        m_collectionList[i].installed = b;
        emit collectionItemUpdated(i, m_collectionList[i]);
        emit dataChanged(this->index(i), this->index(i), {InstalledRole});
    }
}

void LocalDocsModel::updateIndexing(int folder_id, bool b)
{
    for (int i = 0; i < m_collectionList.size(); ++i) {
        if (m_collectionList.at(i).folder_id != folder_id)
            continue;

        m_collectionList[i].indexing = b;
        emit collectionItemUpdated(i, m_collectionList[i]);
        emit dataChanged(this->index(i), this->index(i), {IndexingRole});
    }
}

void LocalDocsModel::updateCurrentDocsToIndex(int folder_id, size_t currentDocsToIndex)
{
    for (int i = 0; i < m_collectionList.size(); ++i) {
        if (m_collectionList.at(i).folder_id != folder_id)
            continue;

        m_collectionList[i].currentDocsToIndex = currentDocsToIndex;
        emit collectionItemUpdated(i, m_collectionList[i]);
        emit dataChanged(this->index(i), this->index(i), {CurrentDocsToIndexRole});
    }
}

void LocalDocsModel::updateTotalDocsToIndex(int folder_id, size_t totalDocsToIndex)
{
    for (int i = 0; i < m_collectionList.size(); ++i) {
        if (m_collectionList.at(i).folder_id != folder_id)
            continue;

        m_collectionList[i].totalDocsToIndex = totalDocsToIndex;
        emit collectionItemUpdated(i, m_collectionList[i]);
        emit dataChanged(this->index(i), this->index(i), {TotalDocsToIndexRole});
    }
}

void LocalDocsModel::subtractCurrentBytesToIndex(int folder_id, size_t subtractedBytes)
{
    for (int i = 0; i < m_collectionList.size(); ++i) {
        if (m_collectionList.at(i).folder_id != folder_id)
            continue;

        m_collectionList[i].currentBytesToIndex -= subtractedBytes;
        emit collectionItemUpdated(i, m_collectionList[i]);
        emit dataChanged(this->index(i), this->index(i), {CurrentBytesToIndexRole});
    }
}

void LocalDocsModel::updateCurrentBytesToIndex(int folder_id, size_t currentBytesToIndex)
{
    for (int i = 0; i < m_collectionList.size(); ++i) {
        if (m_collectionList.at(i).folder_id != folder_id)
            continue;

        m_collectionList[i].currentBytesToIndex = currentBytesToIndex;
        emit collectionItemUpdated(i, m_collectionList[i]);
        emit dataChanged(this->index(i), this->index(i), {CurrentBytesToIndexRole});
    }
}

void LocalDocsModel::updateTotalBytesToIndex(int folder_id, size_t totalBytesToIndex)
{
    for (int i = 0; i < m_collectionList.size(); ++i) {
        if (m_collectionList.at(i).folder_id != folder_id)
            continue;

        m_collectionList[i].totalBytesToIndex = totalBytesToIndex;
        emit collectionItemUpdated(i, m_collectionList[i]);
        emit dataChanged(this->index(i), this->index(i), {TotalBytesToIndexRole});
    }
}

void LocalDocsModel::addCollectionItem(const CollectionItem &item)
{
    beginInsertRows(QModelIndex(), m_collectionList.size(), m_collectionList.size());
    m_collectionList.append(item);
    endInsertRows();
}

void LocalDocsModel::removeFolderById(int folder_id)
{
    for (int i = 0; i < m_collectionList.size();) {
        if (m_collectionList.at(i).folder_id == folder_id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_collectionList.removeAt(i);
            endRemoveRows();
        } else {
            ++i;
        }
    }
}

void LocalDocsModel::removeCollectionPath(const QString &name, const QString &path)
{
    for (int i = 0; i < m_collectionList.size();) {
        if (m_collectionList.at(i).collection == name && m_collectionList.at(i).folder_path == path) {
            beginRemoveRows(QModelIndex(), i, i);
            m_collectionList.removeAt(i);
            endRemoveRows();
        } else {
            ++i;
        }
    }
}

void LocalDocsModel::removeCollectionItem(const QString &collectionName)
{
    for (int i = 0; i < m_collectionList.size();) {
        if (m_collectionList.at(i).collection == collectionName) {
            beginRemoveRows(QModelIndex(), i, i);
            m_collectionList.removeAt(i);
            endRemoveRows();
        } else {
            ++i;
        }
    }
}

void LocalDocsModel::collectionListUpdated(const QList<CollectionItem> &collectionList)
{
    beginResetModel();
    m_collectionList = collectionList;
    endResetModel();
}
