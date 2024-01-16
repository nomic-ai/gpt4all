#ifndef EMBLLM_H
#define EMBLLM_H

#include <QObject>
#include <QThread>
#include "../gpt4all-backend/llmodel.h"

class EmbeddingLLM : public QObject
{
    Q_OBJECT
public:
    EmbeddingLLM();
    virtual ~EmbeddingLLM();

    bool hasModel() const;

public Q_SLOTS:
    std::vector<float> generateEmbeddings(const QString &text);

private:
    bool loadModel();

private:
    LLModel *m_model = nullptr;
};

#endif // EMBLLM_H
