#pragma once

#include "database.h" // IWYU pragma: keep
#include "modellist.h" // IWYU pragma: keep

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVector>

class Chat;
class QDataStream;

class LLModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool restoringFromText READ restoringFromText NOTIFY restoringFromTextChanged)

protected:
    LLModel() = default;

public:
    virtual ~LLModel() = default;

    virtual void destroy() {}
    virtual void regenerateResponse() = 0;
    virtual void resetResponse() = 0;
    virtual void resetContext() = 0;

    virtual void stopGenerating() = 0;

    virtual void loadModelAsync(bool reload = false) = 0;
    virtual void releaseModelAsync(bool unload = false) = 0;
    virtual void requestTrySwitchContext() = 0;
    virtual void setMarkedForDeletion(bool b) = 0;

    virtual void setModelInfo(const ModelInfo &info) = 0;

    virtual bool restoringFromText() const = 0;

    virtual bool serialize(QDataStream &stream, int version, bool serializeKV) = 0;
    virtual bool deserialize(QDataStream &stream, int version, bool deserializeKV, bool discardKV) = 0;
    virtual void setStateFromText(const QVector<QPair<QString, QString>> &stateFromText) = 0;

public Q_SLOTS:
    virtual bool prompt(const QList<QString> &collectionList, const QString &prompt) = 0;
    virtual bool loadModel(const ModelInfo &modelInfo) = 0;
    virtual void modelChangeRequested(const ModelInfo &modelInfo) = 0;
    virtual void generateName() = 0;
    virtual void processSystemPrompt() = 0;

Q_SIGNALS:
    void restoringFromTextChanged();
    void loadedModelInfoChanged();
    void modelLoadingPercentageChanged(float loadingPercentage);
    void modelLoadingError(const QString &error);
    void modelLoadingWarning(const QString &warning);
    void responseChanged(const QString &response);
    void promptProcessing();
    void generatingQuestions();
    void responseStopped(qint64 promptResponseMs);
    void generatedNameChanged(const QString &name);
    void generatedQuestionFinished(const QString &generatedQuestion);
    void stateChanged();
    void threadStarted();
    void trySwitchContextRequested(const ModelInfo &modelInfo);
    void trySwitchContextOfLoadedModelCompleted(int value);
    void requestRetrieveFromDB(const QList<QString> &collections, const QString &text, int retrievalSize, QList<ResultInfo> *results);
    void reportSpeed(const QString &speed);
    void reportDevice(const QString &device);
    void reportFallbackReason(const QString &fallbackReason);
    void databaseResultsChanged(const QList<ResultInfo> &results);
    void modelInfoChanged(const ModelInfo &modelInfo);
};
