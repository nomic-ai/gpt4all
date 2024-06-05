#ifndef EMBEDDINGS_H
#define EMBEDDINGS_H

#include <QObject>

namespace hnswlib {
    template <typename T>
    class HierarchicalNSW;
    class InnerProductSpace;
}

class Embeddings : public QObject
{
    Q_OBJECT
public:
    Embeddings(QObject *parent);
    virtual ~Embeddings();

    bool load();
    bool load(qint64 maxElements);
    bool save();
    void syncDir();
    bool isLoaded() const;
    bool fileExists() const;
    bool resize(qint64 size);

    // Adds the embedding and returns the label used
    bool add(const std::vector<float> &embedding, qint64 label);

    // Removes the embedding at label by marking it as unused
    void remove(qint64 label);

    // Clears the embeddings
    void clear();

    // Performs a nearest neighbor search of the embeddings and returns a vector of labels
    // for the K nearest neighbors of the given embedding
    std::vector<qint64> search(const std::vector<float> &embedding, int K);

private:
    QString m_filePath;
    hnswlib::InnerProductSpace *m_space;
    hnswlib::HierarchicalNSW<float> *m_hnsw;
    bool m_fileExists = false; // we opened and/or created the file
    bool m_filePersisted = false; // dentry has been flushed to disk
};

#endif // EMBEDDINGS_H
