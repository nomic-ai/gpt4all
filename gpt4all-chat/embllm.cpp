#include "embllm.h"
#include "modellist.h"

EmbeddingLLM::EmbeddingLLM()
    : QObject{nullptr}
    , m_model{nullptr}
{
}

EmbeddingLLM::~EmbeddingLLM()
{
    delete m_model;
    m_model = nullptr;
}

bool EmbeddingLLM::loadModel()
{
    const EmbeddingModels *embeddingModels = ModelList::globalInstance()->embeddingModels();
    if (!embeddingModels->count())
        return false;

    const ModelInfo defaultModel = embeddingModels->defaultModelInfo();

    QString filePath = defaultModel.dirpath + defaultModel.filename();
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qWarning() << "WARNING: Could not load sbert because file does not exist";
        m_model = nullptr;
        return false;
    }

    m_model = LLModel::Implementation::construct(filePath.toStdString());
    bool success = m_model->loadModel(filePath.toStdString(), 2048);
    if (!success) {
        qWarning() << "WARNING: Could not load sbert";
        delete m_model;
        m_model = nullptr;
        return false;
    }

    if (m_model->implementation().modelType() != "Bert") {
        qWarning() << "WARNING: Model type is not sbert";
        delete m_model;
        m_model = nullptr;
        return false;
    }
    return true;
}

bool EmbeddingLLM::hasModel() const
{
    return m_model;
}

std::vector<float> EmbeddingLLM::generateEmbeddings(const QString &text)
{
    if (!hasModel() && !loadModel()) {
        qWarning() << "WARNING: Could not load sbert model for embeddings";
        return std::vector<float>();
    }

    Q_ASSERT(hasModel());
    return m_model->embedding(text.toStdString());
}
