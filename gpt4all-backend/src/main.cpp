module;

#include <expected>
#include <memory>

#include <QFuture>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QByteArray>
#include <QString>

#include <QCoro/QCoroNetworkReply>

using namespace Qt::Literals::StringLiterals;

module gpt4all.backend.main;


namespace gpt4all::backend {

auto LLMProvider::getVersion() const -> QCoro::Task<DataOrNetErr<QString>>
{
    QNetworkAccessManager nam;
    std::unique_ptr<QNetworkReply> reply(co_await nam.get(QNetworkRequest(m_baseUrl.resolved(u"/api/version"_s))));
    if (auto err = reply->error())
        co_return std::unexpected(err);

    // TODO(jared): parse JSON here instead of just returning the data
    co_return QString::fromUtf8(reply->readAll());
}

} // namespace gpt4all::backend
