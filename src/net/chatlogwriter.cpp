#include <net/chatlogwriter.h>
#include <core/loghelper.h>

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <boost/filesystem.hpp>
#include <fstream>

using namespace std;

ChatLogWriter::ChatLogWriter(const string &dir, const string &name, int rotateHours)
	: m_rotateSeconds(static_cast<time_t>(rotateHours) * 3600), m_periodStart(0), m_enabled(false)
{
	if (dir.empty() || rotateHours <= 0)
		return;

	boost::filesystem::path logPath(dir);
	boost::system::error_code ec;
	boost::filesystem::create_directories(logPath, ec);
	if (ec) {
		LOG_ERROR("Chat log directory " << logPath.string() << " could not be created: " << ec.message());
		return;
	}
	m_fileName = (logPath / (name + ".log")).string();
	m_enabled = true;
	ReadPeriodStart();
}

bool
ChatLogWriter::IsEnabled() const
{
	return m_enabled;
}

const string &
ChatLogWriter::GetFileName() const
{
	return m_fileName;
}

void
ChatLogWriter::ReadPeriodStart()
{
	// Continue the period of a log left by a previous server run. A first line
	// which cannot be parsed starts a new period now.
	ifstream in(m_fileName.c_str());
	string firstLine;
	if (!in || !getline(in, firstLine))
		return;
	QJsonObject entry = QJsonDocument::fromJson(QByteArray::fromStdString(firstLine)).object();
	m_periodStart = static_cast<time_t>(entry.value("ts").toDouble());
	if (m_periodStart <= 0)
		m_periodStart = time(NULL);
}

void
ChatLogWriter::WriteChatMessage(unsigned playerDbId, const string &playerName, const string &message,
								bool shadowMuted, unsigned gameId, const string &gameName)
{
	if (!m_enabled)
		return;

	QDateTime now = QDateTime::currentDateTimeUtc();
	QJsonObject entry;
	entry["ts"] = static_cast<double>(now.toSecsSinceEpoch());
	entry["time"] = now.toString(Qt::ISODate);
	entry["ch"] = gameId ? "game" : "lobby";
	if (gameId) {
		entry["game"] = static_cast<double>(gameId);
		entry["game_name"] = QString::fromStdString(gameName);
	}
	entry["pid"] = static_cast<double>(playerDbId);
	entry["name"] = QString::fromStdString(playerName);
	entry["msg"] = QString::fromStdString(message);
	if (shadowMuted)
		entry["muted"] = true;
	// Compact JSON escapes line breaks, so every entry is exactly one line.
	string line = QJsonDocument(entry).toJson(QJsonDocument::Compact).toStdString() + "\n";

	boost::mutex::scoped_lock lock(m_mutex);
	const time_t nowSecs = static_cast<time_t>(now.toSecsSinceEpoch());
	if (m_periodStart && nowSecs - m_periodStart >= m_rotateSeconds) {
		boost::system::error_code ec;
		boost::filesystem::rename(m_fileName, m_fileName + ".1", ec);
		if (ec)
			LOG_ERROR("Chat log " << m_fileName << " could not be rotated: " << ec.message());
		m_periodStart = 0;
	}
	ofstream out(m_fileName.c_str(), ios::out | ios::app | ios::binary);
	if (!out) {
		LOG_ERROR("Chat log " << m_fileName << " could not be opened.");
		return;
	}
	out << line;
	if (!m_periodStart)
		m_periodStart = nowSecs;
}
