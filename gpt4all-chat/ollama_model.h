#pragma once

#include "database.h" // IWYU pragma: keep
#include "llmodel.h"
#include "modellist.h" // IWYU pragma: keep

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVector>

class Chat;
class QDataStream;


class OllamaModel : public LLModel
{
    Q_OBJECT

public:
    OllamaModel();
    ~OllamaModel() override = default;

    void regenerateResponse() override;
    void resetResponse() override;
    void resetContext() override;

    void stopGenerating() override;

    void setShouldBeLoaded(bool b) override;
    void requestTrySwitchContext() override;
    void setForceUnloadModel(bool b) override;
    void setMarkedForDeletion(bool b) override;

    void setModelInfo(const ModelInfo &info) override;

    bool restoringFromText() const override;

    bool serialize(QDataStream &stream, int version, bool serializeKV) override;
    bool deserialize(QDataStream &stream, int version, bool deserializeKV, bool discardKV) override;
    void setStateFromText(const QVector<QPair<QString, QString>> &stateFromText) override;

public Q_SLOTS:
    bool prompt(const QList<QString> &collectionList, const QString &prompt) override;
    bool loadDefaultModel() override;
    bool loadModel(const ModelInfo &modelInfo) override;
    void modelChangeRequested(const ModelInfo &modelInfo) override;
    void generateName() override;
    void processSystemPrompt() override;
};
