#ifndef LOCALDOCSMODEL_H
#define LOCALDOCSMODEL_H

#include <QAbstractListModel>
#include "database.h"

class LocalDocsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        CollectionRole = Qt::UserRole + 1,
        FolderPathRole,
        InstalledRole
    };

    explicit LocalDocsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void addCollectionItem(const CollectionItem &item);
    void handleCollectionListUpdated(const QList<CollectionItem> &collectionList);

private:
    QList<CollectionItem> m_collectionList;
};

#endif // LOCALDOCSMODEL_H
