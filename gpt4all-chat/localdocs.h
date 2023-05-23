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

    QList<QString> result() const { return m_retrieveResult; }
    void requestRetrieve(const QList<QString> &collections, const QString &text);

Q_SIGNALS:
    void requestAddFolder(const QString &collection, const QString &path);
    void requestRemoveFolder(const QString &collection, const QString &path);
    void requestRetrieveFromDB(const QList<QString> &collections, const QString &text);
    void receivedResult();
    void localDocsModelChanged();

private Q_SLOTS:
    void handleRetrieveResult(const QList<QString> &result);

private:
    LocalDocsModel *m_localDocsModel;
    Database *m_database;
    QList<QString> m_retrieveResult;

private:
    explicit LocalDocs();
    ~LocalDocs() {}
    friend class MyLocalDocs;
};

#endif // LOCALDOCS_H
