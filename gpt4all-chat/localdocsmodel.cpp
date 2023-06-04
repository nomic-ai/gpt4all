#include "localdocsmodel.h"

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
    }

    return QVariant();
}

QHash<int, QByteArray> LocalDocsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CollectionRole] = "collection";
    roles[FolderPathRole] = "folder_path";
    roles[InstalledRole] = "installed";
    return roles;
}

void LocalDocsModel::addCollectionItem(const CollectionItem &item)
{
    beginInsertRows(QModelIndex(), m_collectionList.size(), m_collectionList.size());
    m_collectionList.append(item);
    endInsertRows();
}

void LocalDocsModel::handleCollectionListUpdated(const QList<CollectionItem> &collectionList)
{
    beginResetModel();
    m_collectionList = collectionList;
    endResetModel();
}
