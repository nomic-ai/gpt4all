#ifndef LOCALDOCS_H
#define LOCALDOCS_H

#include "localdocsmodel.h"
#include "database.h"

#include <QObject>

class LocalDocs : public QObject
{
    Q_OBJECT
    Q_PROPERTY(LocalDocsModel *localDocsModel READ localDocsModel NOTIFY localDocsModelChanged)

public:
    static LocalDocs *globalInstance();

    LocalDocsModel *localDocsModel() const { return m_localDocsModel; }

    Q_INVOKABLE void addFolder(const QString &collection, const QString &path);
    Q_INVOKABLE void removeFolder(const QString &collection, const QString &path);

    Database *database() const { return m_database; }

public Q_SLOTS:
    void handleChunkSizeChanged();
    void aboutToQuit();

Q_SIGNALS:
    void requestAddFolder(const QString &collection, const QString &path);
    void requestRemoveFolder(const QString &collection, const QString &path);
    void requestChunkSizeChange(int chunkSize);
    void localDocsModelChanged();

private:
    LocalDocsModel *m_localDocsModel;
    Database *m_database;

private:
    explicit LocalDocs();
    friend class MyLocalDocs;
};

#endif // LOCALDOCS_H
