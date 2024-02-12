#ifndef LOCALDOCSMODEL_H
#define LOCALDOCSMODEL_H

#include <QAbstractListModel>
#include "database.h"

class LocalDocsCollectionsModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit LocalDocsCollectionsModel(QObject *parent);

public Q_SLOTS:
    void setCollections(const QList<QString> &collections);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QList<QString> m_collections;
};

class LocalDocsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        CollectionRole = Qt::UserRole + 1,
        FolderPathRole,
        InstalledRole,
        IndexingRole,
        ErrorRole,
        CurrentDocsToIndexRole,
        TotalDocsToIndexRole,
        CurrentBytesToIndexRole,
        TotalBytesToIndexRole,
        CurrentEmbeddingsToIndexRole,
        TotalEmbeddingsToIndexRole
    };

    explicit LocalDocsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void updateInstalled(int folder_id, bool b);
    void updateIndexing(int folder_id, bool b);
    void updateError(int folder_id, const QString &error);
    void updateCurrentDocsToIndex(int folder_id, size_t currentDocsToIndex);
    void updateTotalDocsToIndex(int folder_id, size_t totalDocsToIndex);
    void subtractCurrentBytesToIndex(int folder_id, size_t subtractedBytes);
    void updateCurrentBytesToIndex(int folder_id, size_t currentBytesToIndex);
    void updateTotalBytesToIndex(int folder_id, size_t totalBytesToIndex);
    void updateCurrentEmbeddingsToIndex(int folder_id, size_t currentBytesToIndex);
    void updateTotalEmbeddingsToIndex(int folder_id, size_t totalBytesToIndex);
    void addCollectionItem(const CollectionItem &item);
    void removeFolderById(int folder_id);
    void removeCollectionPath(const QString &name, const QString &path);
    void removeCollectionItem(const QString &collectionName);
    void collectionListUpdated(const QList<CollectionItem> &collectionList);

private:
    template<typename T>
    void updateField(int folder_id, T value,
        const std::function<void(CollectionItem&, T)>& updater,
        const QVector<int>& roles);

private:
    QList<CollectionItem> m_collectionList;
};

#endif // LOCALDOCSMODEL_H
