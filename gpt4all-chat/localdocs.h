#ifndef LOCALDOCS_H
#define LOCALDOCS_H

#include "localdocsmodel.h"
#include "database.h"

#include <QObject>

class LocalDocs : public QObject
{
    Q_OBJECT
    Q_PROPERTY(LocalDocsModel *localDocsModel READ localDocsModel NOTIFY localDocsModelChanged)
    Q_PROPERTY(int chunkSize READ chunkSize WRITE setChunkSize NOTIFY chunkSizeChanged)
    Q_PROPERTY(int retrievalSize READ retrievalSize WRITE setRetrievalSize NOTIFY retrievalSizeChanged)

public:
    static LocalDocs *globalInstance();

    LocalDocsModel *localDocsModel() const { return m_localDocsModel; }

    Q_INVOKABLE void addFolder(const QString &collection, const QString &path);
    Q_INVOKABLE void removeFolder(const QString &collection, const QString &path);

    Database *database() const { return m_database; }

    int chunkSize() const;
    void setChunkSize(int chunkSize);

    int retrievalSize() const;
    void setRetrievalSize(int retrievalSize);

Q_SIGNALS:
    void requestAddFolder(const QString &collection, const QString &path);
    void requestRemoveFolder(const QString &collection, const QString &path);
    void requestChunkSizeChange(int chunkSize);
    void localDocsModelChanged();
    void chunkSizeChanged();
    void retrievalSizeChanged();

private:
    int m_chunkSize;
    int m_retrievalSize;
    LocalDocsModel *m_localDocsModel;
    Database *m_database;

private:
    explicit LocalDocs();
    ~LocalDocs() {}
    friend class MyLocalDocs;
};

#endif // LOCALDOCS_H
