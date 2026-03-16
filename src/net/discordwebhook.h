#ifndef _DISCORDWEBHOOK_H_
#define _DISCORDWEBHOOK_H_

#include <string>

class DiscordWebhookSender
{
public:
	explicit DiscordWebhookSender(const std::string &webhookUrl);

	void SendChatMessage(const std::string &playerName, const std::string &message);
	bool IsEnabled() const;

private:
	std::string m_webhookUrl;
};

#endif
