#include <QString>
#include <QDataStream>
#include <QtGlobal>

#include <fmt/format.h>

#define THROW_IF_BAD(stream)                                                        \
    do {                                                                            \
        if (auto status = (stream).status(); status != QDataStream::Status::OK)     \
            throw std::runtime_error(fmt::format("bad stream status: {}", status)); \
    } while (0)                                                                     \

inline namespace MessageEnums {
    Q_NAMESPACE
    // for DataLake
    enum class MessageRating : quint8 { Unrated = 0, Positive = 1, Negative = 2, Max = Negative };
    Q_ENUM_NS(MessageRating)
}

// TODO(Adam): Maybe we should include the model name here as well as timestamp?
class MessageContent {
    Q_GADGET
    Q_PROPERTY(QString role    READ   role    CONSTANT)
    Q_PROPERTY(QString content MEMBER content)

protected:
    enum class Type : quint8 { Prompt = 0, Response = 1, Max = Response };

public:
    virtual QString role() const = 0;

    friend auto deserialize(QDataStream &stream, ChatModel *model) -> std::unique_ptr<MessageContent>
    {
        union { quint8 u8; };
        THROW_IF_BAD(stream);

        stream >> u8; // version
        THROW_IF_BAD(stream);
        if (u8 > VERSION)
            throw std::invalid_argument(fmt::format("unknown version: {}", u8));

        stream >> u8; // type
        THROW_IF_BAD(stream);
        if (u8 > Type::Max)
            throw std::invalid_argument(fmt::format("unknown type: {}", u8));
        auto type = Type(u8);

        std::unique_ptr<MessageContent> result;
        switch (type) {
            case Prompt:   result = std::make_unique<PromptContent>  ();      break;
            case Response: result = std::make_unique<ResponseContent>(model); break;
        }

        stream >> result->content;
        THROW_IF_BAD(stream);
        // TODO: add more common fields as needed

        result->deserializeInternal(stream, version);
        return result;
    }

protected:
    virtual void deserializeInternal(QDataStream &stream, quint32 version) = 0;

public:
    QString content;

private:
    static quint8 VERSION = 0;
};
Q_DECLARE_METATYPE(MessageContent)

class PromptContent final : public MessageContent {
    Q_GADGET
    Q_PROPERTY(QList<ResultInfo>       sources               MEMBER sources)
    Q_PROPERTY(QList<ResultInfo>       consolidatedSources   MEMBER consolidatedSources)
    Q_PROPERTY(QList<PromptAttachment> promptAttachments     MEMBER promptAttachments)
    Q_PROPERTY(QString                 promptPlusAttachments READ   promptPlusAttachments)

public:
    QString role() const override { return u"user"_s; }

    QString promptPlusAttachments() const
    {
        if (!promptAttachments.isEmpty()) {
            QStringList items;
            for (auto &attached : std::as_const(promptAttachments))
                items << attached.processedContent();
            items << content;
            return items.join("\n\n");
        }
        return content;
    }

protected:
    void deserializeInternal(QDataStream &stream, quint32 version) override
    {
        Q_UNUSED(version); // only v0 exists currently
        // TODO: ...
    }
};
Q_DECLARE_METATYPE(PromptContent)

class ResponseContent final : public MessageContent {
    Q_GADGET
    Q_PROPERTY(QString newResponse MEMBER newResponse) // for DataLake
    Q_PROPERTY(bool currentResponse READ currentResponse)
    Q_PROPERTY(bool stopped MEMBER stopped) // for DataLake
    Q_PROPERTY(MessageRating rating MEMBER rating) // for DataLake

public:
    explicit ResponseContent(ChatModel *model)
        : m_model(model) {}

    QString role() const override { return u"assistant"_s; }
    bool currentResponse() const { return this == m_model->currentResponse(); }

protected:
    void deserializeInternal(QDataStream &stream, quint32 version) override
    {
        Q_UNUSED(version); // only v0 exists currently

        stream >> newResponse;
        THROW_IF_BAD(stream);

        stream >> stopped;
        THROW_IF_BAD(stream);

        stream >> rating;
        THROW_IF_BAD(stream);
    }

public:
    QString newResponse;
    bool stopped = false;
    MessageRating rating = MessageRating::Unrated;

private:
    ChatModel *m_model;
};
Q_DECLARE_METATYPE(ResponseContent)
