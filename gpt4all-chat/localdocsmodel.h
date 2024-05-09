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
        ForceIndexingRole,
        CurrentDocsToIndexRole,
        TotalDocsToIndexRole,
        CurrentBytesToIndexRole,
        TotalBytesToIndexRole,
        CurrentEmbeddingsToIndexRole,
        TotalEmbeddingsToIndexRole,
        TotalDocsRole,
        TotalWordsRole,
        TotalTokensRole,
        LastUpdateRole,
        FileCurrentlyProcessingRole,
        EmbeddingModelRole
    };

    explicit LocalDocsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void updateCollectionItem(const CollectionItem&);
    void addCollectionItem(const CollectionItem &item);
    void removeFolderById(int folder_id);
    void removeCollectionPath(const QString &name, const QString &path);
    void collectionListUpdated(const QList<CollectionItem> &collectionList);

private:
    void removeCollectionIf(std::function<bool(CollectionItem)> const &predicate);
    QList<CollectionItem> m_collectionList;
};

#endif // LOCALDOCSMODEL_H
