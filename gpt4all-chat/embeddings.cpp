#include "embeddings.h"

#include <QFile>
#include <QFileInfo>
#include <QDebug>

#include "mysettings.h"
#include "hnswlib/hnswlib.h"

#define EMBEDDINGS_VERSION 0

const int s_dim = 384;              // Dimension of the elements
const int s_ef_construction = 200;  // Controls index search speed/build speed tradeoff
const int s_M = 16;                 // Tightly connected with internal dimensionality of the data
                                    // strongly affects the memory consumption

Embeddings::Embeddings(QObject *parent)
    : QObject(parent)
    , m_space(nullptr)
    , m_hnsw(nullptr)
{
    m_filePath = MySettings::globalInstance()->modelPath()
        + QString("embeddings_v%1.dat").arg(EMBEDDINGS_VERSION);
}

Embeddings::~Embeddings()
{
    delete m_hnsw;
    m_hnsw = nullptr;
    delete m_space;
    m_space = nullptr;
}

bool Embeddings::load()
{
    QFileInfo info(m_filePath);
    if (!info.exists()) {
        qWarning() << "ERROR: loading embeddings file does not exist" << m_filePath;
        return false;
    }

    if (!info.isReadable()) {
        qWarning() << "ERROR: loading embeddings file is not readable" << m_filePath;
        return false;
    }

    if (!info.isWritable()) {
        qWarning() << "ERROR: loading embeddings file is not writeable" << m_filePath;
        return false;
    }

    try {
        m_space = new hnswlib::InnerProductSpace(s_dim);
        m_hnsw = new hnswlib::HierarchicalNSW<float>(m_space, m_filePath.toStdString(), s_M, s_ef_construction);
    } catch (const std::exception &e) {
        qWarning() << "ERROR: could not load hnswlib index:" << e.what();
        return false;
    }
    return isLoaded();
}

bool Embeddings::load(qint64 maxElements)
{
    try {
        m_space = new hnswlib::InnerProductSpace(s_dim);
        m_hnsw = new hnswlib::HierarchicalNSW<float>(m_space, maxElements, s_M, s_ef_construction);
    } catch (const std::exception &e) {
        qWarning() << "ERROR: could not create hnswlib index:" << e.what();
        return false;
    }
    return isLoaded();
}

bool Embeddings::save()
{
    if (!isLoaded())
        return false;
    try {
        m_hnsw->saveIndex(m_filePath.toStdString());
    } catch (const std::exception &e) {
        qWarning() << "ERROR: could not save hnswlib index:" << e.what();
        return false;
    }
    return true;
}

bool Embeddings::isLoaded() const
{
    return m_hnsw != nullptr;
}

bool Embeddings::fileExists() const
{
    QFileInfo info(m_filePath);
    return info.exists();
}

bool Embeddings::resize(qint64 size)
{
    if (!isLoaded()) {
        qWarning() << "ERROR: attempting to resize an embedding when the embeddings are not open!";
        return false;
    }

    Q_ASSERT(m_hnsw);
    try {
        m_hnsw->resizeIndex(size);
    } catch (const std::exception &e) {
        qWarning() << "ERROR: could not resize hnswlib index:" << e.what();
        return false;
    }
    return true;
}

bool Embeddings::add(const std::vector<float> &embedding, qint64 label)
{
    if (!isLoaded()) {
        bool success = load(500);
        if (!success) {
            qWarning() << "ERROR: attempting to add an embedding when the embeddings are not open!";
            return false;
        }
    }

    Q_ASSERT(m_hnsw);
    if (m_hnsw->cur_element_count + 1 > m_hnsw->max_elements_) {
        if (!resize(m_hnsw->max_elements_ + 500)) {
            return false;
        }
    }

    if (embedding.empty())
        return false;

    try {
        m_hnsw->addPoint(embedding.data(), label, false);
    } catch (const std::exception &e) {
        qWarning() << "ERROR: could not add embedding to hnswlib index:" << e.what();
        return false;
    }
    return true;
}

void Embeddings::remove(qint64 label)
{
    if (!isLoaded()) {
        qWarning() << "ERROR: attempting to remove an embedding when the embeddings are not open!";
        return;
    }

    Q_ASSERT(m_hnsw);
    try {
        m_hnsw->markDelete(label);
    } catch (const std::exception &e) {
        qWarning() << "ERROR: could not add remove embedding from hnswlib index:" << e.what();
    }
}

void Embeddings::clear()
{
    delete m_hnsw;
    m_hnsw = nullptr;
    delete m_space;
    m_space = nullptr;
}

std::vector<qint64> Embeddings::search(const std::vector<float> &embedding, int K)
{
    if (!isLoaded())
        return {};

    Q_ASSERT(m_hnsw);
    std::priority_queue<std::pair<float, hnswlib::labeltype>> result;
    try {
        result = m_hnsw->searchKnn(embedding.data(), K);
    } catch (const std::exception &e) {
        qWarning() << "ERROR: could not search hnswlib index:" << e.what();
        return {};
    }

    std::vector<qint64> neighbors;
    neighbors.reserve(K);

    while(!result.empty()) {
        neighbors.push_back(result.top().second);
        result.pop();
    }

    // Reverse the neighbors, as the top of the priority queue is the farthest neighbor.
    std::reverse(neighbors.begin(), neighbors.end());

    return neighbors;
}
