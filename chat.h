#ifndef CHAT_H
#define CHAT_H

#include <QObject>
#include <QtQml>

#include "chatmodel.h"
#include "network.h"

class Chat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id NOTIFY idChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(ChatModel *chatModel READ chatModel NOTIFY chatModelChanged)
    QML_ELEMENT
    QML_UNCREATABLE("Only creatable from c++!")

public:
    explicit Chat(QObject *parent = nullptr) : QObject(parent)
    {
        m_id = Network::globalInstance()->generateUniqueId();
        m_name = tr("New Chat");
        m_chatModel = new ChatModel(this);
    }

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    ChatModel *chatModel() { return m_chatModel; }

Q_SIGNALS:
    void idChanged();
    void nameChanged();
    void chatModelChanged();

private:
    QString m_id;
    QString m_name;
    ChatModel *m_chatModel;
};

#endif // CHAT_H
