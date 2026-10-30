#ifndef _CHATLOGWRITER_H_
#define _CHATLOGWRITER_H_

#include <string>
#include <ctime>
#include <boost/thread/mutex.hpp>

// Appends chat lines for moderation to <dir>/<name>.log, one JSON object per
// line (JSON Lines), meant to be read by the web server:
//
//   {"ts":1793368992,"time":"2026-10-30T14:03:12Z","ch":"lobby","pid":4711,"name":"Foo","msg":"hi"}
//
// "pid" is player.player_id (0 without a database), "muted":true marks lines a
// shadow-muted player sent (nobody but the sender saw them). Game chat is
// written with "ch":"game" plus "game" (game id) and "game_name".
//
// The file is rotated once its first line is older than the rotation period:
// it is renamed to <name>.log.1, replacing the previous one, so one to two
// periods of chat are kept.
class ChatLogWriter
{
public:
	// rotateHours <= 0 or an empty dir disables the log.
	ChatLogWriter(const std::string &dir, const std::string &name, int rotateHours);

	bool IsEnabled() const;
	const std::string &GetFileName() const;

	// gameId 0 = lobby chat.
	void WriteChatMessage(unsigned playerDbId, const std::string &playerName, const std::string &message,
						  bool shadowMuted, unsigned gameId = 0, const std::string &gameName = std::string());

private:
	void ReadPeriodStart();

	std::string m_fileName;
	std::time_t m_rotateSeconds;
	std::time_t m_periodStart;
	bool m_enabled;
	boost::mutex m_mutex;
};

#endif
