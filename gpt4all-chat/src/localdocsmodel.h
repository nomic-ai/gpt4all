#ifndef LOCALDOCSMODEL_H
#define LOCALDOCSMODEL_H

#include "database.h"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSortFilterProxyModel>
#include <QString>
#include <QVariant>
#include <Qt>

#include <functional>

class LocalDocsCollectionsModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int updatingCount READ updatingCount NOTIFY updatingCountChanged)

public:
    explicit LocalDocsCollectionsModel(QObject *parent);
    int count() const { return rowCount(); }
    int updatingCount() const;

public Q_SLOTS:
    void setCollections(const QList<QString> &collections);

Q_SIGNALS:
    void countChanged();
    void updatingCountChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private Q_SLOTS:
    void maybeTriggerUpdatingCountChanged();

private:
    QList<QString> m_collections;
    int m_updatingCount = 0;
};

class LocalDocsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

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
        StartUpdateRole,
        LastUpdateRole,
        FileCurrentlyProcessingRole,
        EmbeddingModelRole,
        UpdatingRole
    };

    explicit LocalDocsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return rowCount(); }

public Q_SLOTS:
    void updateCollectionItem(const CollectionItem&);
    void addCollectionItem(const CollectionItem &item);
    void removeFolderById(const QString &collection, int folder_id);
    void removeCollectionPath(const QString &name, const QString &path);
    void collectionListUpdated(const QList<CollectionItem> &collectionList);

Q_SIGNALS:
    void countChanged();
    void updatingChanged(const QString &collection);

private:
    void removeCollectionIf(std::function<bool(CollectionItem)> const &predicate);
    QList<CollectionItem> m_collectionList;
};

#endif // LOCALDOCSMODEL_H
