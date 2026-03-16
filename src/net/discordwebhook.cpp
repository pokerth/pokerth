#include <net/discordwebhook.h>
#include <core/loghelper.h>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QUrl>
#include <thread>

DiscordWebhookSender::DiscordWebhookSender(const std::string &webhookUrl)
	: m_webhookUrl(webhookUrl)
{
}

bool
DiscordWebhookSender::IsEnabled() const
{
	return !m_webhookUrl.empty();
}

void
DiscordWebhookSender::SendChatMessage(const std::string &playerName, const std::string &message)
{
	if (!IsEnabled()) {
		return;
	}

	std::string url = m_webhookUrl;
	std::string content = "**" + playerName + ":** " + message;

	// Fire-and-forget in a detached thread with its own Qt event loop,
	// because the server main loop does not run QCoreApplication::exec().
	std::thread([url, content]() {
		QNetworkAccessManager manager;
		QEventLoop loop;

		QJsonObject json;
		json["content"] = QString::fromStdString(content);

		QNetworkRequest request(QUrl(QString::fromStdString(url)));
		request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

		QNetworkReply *reply = manager.post(request, QJsonDocument(json).toJson(QJsonDocument::Compact));
		QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
		loop.exec();

		if (reply->error() != QNetworkReply::NoError) {
			LOG_ERROR("Discord webhook failed: " << reply->errorString().toStdString());
		}
		reply->deleteLater();
	}).detach();
}
