/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2016 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#include <dbofficial/serverdbthread.h>
#include <dbofficial/asyncdbauth.h>
#include <dbofficial/asyncdblogin.h>
#include <dbofficial/asyncdbavatarblacklist.h>
#include <dbofficial/asyncdbcreategame.h>
#include <dbofficial/asyncdbendgame.h>
#include <dbofficial/asyncdbgameplace.h>
#include <dbofficial/asyncdbupdatescore.h>
#include <dbofficial/asyncdbreportavatar.h>
#include <dbofficial/asyncdbreportgame.h>
#include <dbofficial/asyncdbadminplayers.h>
#include <dbofficial/asyncdbblockplayer.h>
#include <dbofficial/asyncdbplayerlastgames.h>
#include <dbofficial/compositeasyncdbquery.h>
#include <dbofficial/db_table_defs.h>
#include <ctime>
#include <sstream>
#include <dbofficial/mysqlpp_compat.h>

#include <core/loghelper.h> // @TODO: remove in productive
#include <boost/date_time/posix_time/posix_time.hpp>

// Interval (in seconds) at which the DB thread sends a keepalive ping
// to prevent the MySQL server from closing idle connections.
// 30s is well below typical MySQL wait_timeout values.
#define DB_KEEPALIVE_INTERVAL_SEC		30

#define QUERY_NICK_PREPARE				"nick_template"
#define QUERY_LOGIN_PREPARE				"login_template"
#define QUERY_AVATAR_BLACKLIST_PREPARE	"avatar_blacklist_template"
#define QUERY_CREATE_GAME_PREPARE		"create_game_template"
#define QUERY_END_GAME_PREPARE			"end_game_template"
#define QUERY_GAME_PLAYER_PREPARE		"game_player_template"
#define QUERY_UPDATE_SCORE_PREPARE		"update_score_template"
#define QUERY_REPORT_AVATAR_PREPARE		"report_avatar_template"
#define QUERY_REPORT_GAME_PREPARE		"report_game_template"
#define QUERY_ADMIN_PLAYER_PREPARE		"admin_player_template"
#define QUERY_BLOCK_PLAYER_PREPARE		"block_player_template"
#define QUERY_PLAYER_LASTGAMES_PREPARE	"player_lastgames_template"

using namespace std;

struct DBConnectionData {
	DBConnectionData() : conn(false) {}
	string host;
	string user;
	string pwd;
	string database;
	string encryptionKey;
	mysqlpp::Connection conn;
};

ServerDBThread::ServerDBThread(ServerDBCallback &cb, boost::shared_ptr<boost::asio::io_context> ioService)
	: m_ioService(ioService), m_semaphore(0), m_callback(cb), m_isConnected(false), m_permanentError(false), m_previouslyConnected(false)
{
	m_connData.reset(new DBConnectionData);
}

ServerDBThread::~ServerDBThread()
{
}

void
ServerDBThread::SignalTermination()
{
	Thread::SignalTermination();
	m_semaphore.post();
}

void
ServerDBThread::Init(const string &host, const string &user, const string &pwd,
					 const string &database, const string &encryptionKey)
{
	m_connData->host = host;
	m_connData->user = user;
	m_connData->pwd = pwd;
	m_connData->database = database;
	m_connData->encryptionKey = encryptionKey;
}

void
ServerDBThread::Start()
{
	Run();
}

void
ServerDBThread::Stop()
{
	SignalTermination();
	this->Join(THREAD_WAIT_INFINITE);
}

void
ServerDBThread::AsyncPlayerLogin(unsigned requestId, const string &playerName)
{
	// Always queue the request, even if not currently connected.
	// The DB thread will process it once connection is re-established.
	list<string> params;
	params.push_back(m_connData->encryptionKey);
	params.push_back(playerName);
	boost::shared_ptr<AsyncDBQuery> asyncQuery(
		new AsyncDBAuth(
			requestId,
			QUERY_NICK_PREPARE,
			params));

	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		m_asyncQueue.push(asyncQuery);
	}
	m_semaphore.post();
}

void
ServerDBThread::AsyncCheckAvatarBlacklist(unsigned requestId, const std::string &avatarHash)
{
	// Always queue the request, even if not currently connected.
	// The DB thread will process it once connection is re-established.
	list<string> params;
	params.push_back(avatarHash);
	boost::shared_ptr<AsyncDBQuery> asyncQuery(
		new AsyncDBAvatarBlacklist(
			requestId,
			QUERY_AVATAR_BLACKLIST_PREPARE,
			params));

	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		m_asyncQueue.push(asyncQuery);
	}
	m_semaphore.post();
}

void
ServerDBThread::PlayerPostLogin(DB_id playerId, const std::string &avatarHash, const std::string &avatarType)
{
	list<string> params;
	params.push_back(mysqlpp::DateTime(time(NULL)));
	params.push_back(avatarHash);
	params.push_back(avatarType);
	ostringstream paramStream;
	paramStream << playerId;
	params.push_back(paramStream.str());
	boost::shared_ptr<AsyncDBQuery> asyncQuery(
		new AsyncDBLogin(
			playerId,
			QUERY_LOGIN_PREPARE,
			params));

	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		m_asyncQueue.push(asyncQuery);
	}
	m_semaphore.post();
}

void
ServerDBThread::PlayerLogout(DB_id /*playerId*/)
{
}

void
ServerDBThread::AsyncCreateGame(unsigned requestId, const string &gameName)
{
	list<string> params;
	params.push_back(gameName);
	params.push_back(mysqlpp::DateTime(time(NULL)));
	boost::shared_ptr<AsyncDBQuery> asyncQuery(
		new AsyncDBCreateGame(
			requestId,
			QUERY_CREATE_GAME_PREPARE,
			params));

	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		m_asyncQueue.push(asyncQuery);
	}
	
	m_semaphore.post();
}

void
ServerDBThread::SetGamePlayerPlace(unsigned requestId, DB_id playerId, unsigned place)
{
	// The game id param is added later (during init of the async op), because it may be unknown.
	list<string> params;
	ostringstream paramStream;
	paramStream << playerId;
	params.push_back(paramStream.str());
	paramStream.str("");
	paramStream << place;
	params.push_back(paramStream.str());
	boost::shared_ptr<AsyncDBQuery> asyncQuery(
		new AsyncDBGamePlace(
			requestId,
			QUERY_GAME_PLAYER_PREPARE,
			params));

	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		m_asyncQueue.push(asyncQuery);
	}
	// m_semaphore.post();
	m_semaphore.post();
}

void
ServerDBThread::SetPlayerLastGames(unsigned requestId, DB_id playerId, std::vector<long> last_games, std::string playerIp)
{

	std::ostringstream oss;
    std::copy(last_games.begin(), last_games.end(), std::ostream_iterator<int>(oss, ","));
    std::string last_gamesFieldValue( oss.str() );
	list<string> params;
	ostringstream paramStream;
	params.push_back(last_gamesFieldValue);
	params.push_back(playerIp);
	paramStream << playerId;
	params.push_back(paramStream.str());
	boost::shared_ptr<AsyncDBQuery> asyncQuery(
		new AsyncDBPlayerLastGames(
			requestId,
			QUERY_PLAYER_LASTGAMES_PREPARE,
			params));
	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		m_asyncQueue.push(asyncQuery);
	}
	m_semaphore.post();
}

void
ServerDBThread::EndGame(unsigned requestId)
{
	// Set the end time of the game.
	{
		list<string> params;
		params.push_back(mysqlpp::DateTime(time(NULL)));
		boost::shared_ptr<AsyncDBQuery> asyncQuery(
			new AsyncDBEndGame(
				requestId,
				QUERY_END_GAME_PREPARE,
				params));

		{
			boost::mutex::scoped_lock lock(m_asyncQueueMutex);
			m_asyncQueue.push(asyncQuery);
		}
		
		m_semaphore.post();
	}
	// Update the player scores.
	{
		list<string> params;
		boost::shared_ptr<AsyncDBQuery> asyncQuery(
			new AsyncDBUpdateScore(
				requestId,
				QUERY_UPDATE_SCORE_PREPARE,
				params));

		{
			boost::mutex::scoped_lock lock(m_asyncQueueMutex);
			m_asyncQueue.push(asyncQuery);
		}
		
		m_semaphore.post();
	}
}

void
ServerDBThread::AsyncReportAvatar(unsigned requestId, unsigned replyId, DB_id reportedPlayerId, const std::string &avatarHash, const std::string &avatarType, DB_id *byPlayerId)
{
	list<string> params;
	ostringstream paramStream;
	paramStream << reportedPlayerId;
	params.push_back(paramStream.str());
	params.push_back(avatarHash);
	params.push_back(avatarType);
	if (byPlayerId) {
		paramStream.str("");
		paramStream << *byPlayerId;
		params.push_back(paramStream.str());
	} else {
		params.push_back("NULL");
	}
	params.push_back(mysqlpp::DateTime(time(NULL)));

	boost::shared_ptr<AsyncDBQuery> asyncQuery(
		new AsyncDBReportAvatar(
			requestId,
			replyId,
			QUERY_REPORT_AVATAR_PREPARE,
			params));

	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		m_asyncQueue.push(asyncQuery);
	}
	
	m_semaphore.post();
}

void
ServerDBThread::AsyncReportGame(unsigned requestId, unsigned replyId, DB_id *creatorPlayerId, unsigned gameId, const std::string &gameName, DB_id *byPlayerId)
{
	list<string> params;
	ostringstream paramStream;
	if (creatorPlayerId) {
		paramStream << *creatorPlayerId;
		params.push_back(paramStream.str());
	} else {
		params.push_back("NULL");
	}
	params.push_back(gameName);
	if (byPlayerId) {
		paramStream.str("");
		paramStream << *byPlayerId;
		params.push_back(paramStream.str());
	} else {
		params.push_back("NULL");
	}
	params.push_back(mysqlpp::DateTime(time(NULL)));

	boost::shared_ptr<AsyncDBQuery> asyncQuery(
		new AsyncDBReportGame(
			requestId,
			replyId,
			gameId,
			QUERY_REPORT_GAME_PREPARE,
			params));

	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		m_asyncQueue.push(asyncQuery);
	}

	
	m_semaphore.post();
}

void
ServerDBThread::AsyncQueryAdminPlayers(unsigned requestId)
{
	boost::shared_ptr<AsyncDBQuery> asyncQuery(
		new AsyncDBAdminPlayers(
			requestId,
			QUERY_ADMIN_PLAYER_PREPARE));
	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		m_asyncQueue.push(asyncQuery);
	}

	
	m_semaphore.post();
}

void
ServerDBThread::AsyncBlockPlayer(unsigned requestId, unsigned replyId, DB_id playerId, int valid, int active)
{
	list<string> params;
	ostringstream paramStream;
	paramStream << valid;
	params.push_back(paramStream.str());
	paramStream.str("");
	paramStream << active;
	params.push_back(paramStream.str());
	paramStream.str("");
	paramStream << playerId;
	params.push_back(paramStream.str());
	boost::shared_ptr<AsyncDBQuery> asyncQuery(
		new AsyncDBBlockPlayer(
			requestId,
			replyId,
			QUERY_BLOCK_PLAYER_PREPARE,
			params));

	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		m_asyncQueue.push(asyncQuery);
	}
	
	m_semaphore.post();
}

bool
ServerDBThread::IsConnected() const
{
	boost::mutex::scoped_lock lock(m_isConnectedMutex);
	return m_isConnected;
}

void
ServerDBThread::Main()
{
	while (!ShouldTerminate() && !HasPermanentError()) {
		if (HasDBConnection()) {
			SetConnected(true);
			// Use a timed wait so we can send periodic keepalive pings
			// to prevent the MySQL server from closing idle connections
			// (default wait_timeout is often 28800s, but external/cloud
			// MySQL hosts frequently use much shorter values like 60-300s).
			bool hasWork = m_semaphore.timed_wait(
				boost::posix_time::microsec_clock::universal_time()
				+ boost::posix_time::seconds(DB_KEEPALIVE_INTERVAL_SEC));
			if (hasWork) {
				// Validate the connection is still alive before executing.
				// mysqlpp::Connection::connected() only checks local state;
				// ping() actually talks to the server and auto-reconnects
				// if the connection was dropped (e.g. by MySQL wait_timeout).
				if (!m_connData->conn.ping()) {
					LOG_ERROR("DB connection lost before query execution, reconnecting...");
					m_connData->conn.disconnect();
					continue;
				}
				HandleNextQuery();
			} else {
				// Timed out with no work — send a keepalive ping.
				if (!m_connData->conn.ping()) {
					LOG_ERROR("DB keepalive ping failed, reconnecting...");
					m_connData->conn.disconnect();
				}
			}
		} else {
			SetConnected(false);
			EstablishDBConnection();
		}
	}
	m_connData->conn.disconnect();
}

bool
ServerDBThread::HasPermanentError() const
{
	return m_permanentError;
}

bool
ServerDBThread::HasDBConnection() const
{
	return m_connData->conn.connected();
}

void
ServerDBThread::EstablishDBConnection()
{
	m_connData->conn.set_option(new mysqlpp::SetCharsetNameOption("utf8"));
	if (!m_connData->conn.connect(
				m_connData->database.c_str(), m_connData->host.c_str(), m_connData->user.c_str(), m_connData->pwd.c_str())) {
		boost::asio::post(*m_ioService, boost::bind(&ServerDBCallback::ConnectFailed, &m_callback, m_connData->conn.error()));
		if (!m_previouslyConnected)
			m_permanentError = true;
		else
			Msleep(250);
	} else {
		mysqlpp::Query prepareNick = m_connData->conn.query();
		/*
		prepareNick
				<< "PREPARE " QUERY_NICK_PREPARE " FROM " << mysqlpp::quote
				<< "SELECT " DB_TABLE_PLAYER_COL_ID ", AES_DECRYPT(" DB_TABLE_PLAYER_COL_PASSWORD ", ?), " DB_TABLE_PLAYER_COL_VALID ", TRIM(" DB_TABLE_PLAYER_COL_COUNTRY "), " DB_TABLE_PLAYER_COL_LASTLOGIN ", " DB_TABLE_PLAYER_COL_ACTIVE " FROM " DB_TABLE_PLAYER " WHERE BINARY " DB_TABLE_PLAYER_COL_USERNAME " = ?";
		*/
		prepareNick
				<< "PREPARE " QUERY_NICK_PREPARE " FROM " << mysqlpp::quote
				<< "SELECT " DB_TABLE_PLAYER_COL_ID ", AES_DECRYPT(" DB_TABLE_PLAYER_COL_PASSWORD ", ?), " DB_TABLE_PLAYER_COL_VALID ", TRIM(" DB_TABLE_PLAYER_COL_COUNTRY "), " DB_TABLE_PLAYER_COL_LASTLOGIN ", " DB_TABLE_PLAYER_COL_LASTGAMES ", " DB_TABLE_PLAYER_COL_LASTIP ", " DB_TABLE_PLAYER_COL_ACTIVE " FROM " DB_TABLE_PLAYER " WHERE " DB_TABLE_PLAYER_COL_USERNAME " = ?";

		mysqlpp::Query prepareAvatarBlacklist = m_connData->conn.query();
		prepareAvatarBlacklist
				<< "PREPARE " QUERY_AVATAR_BLACKLIST_PREPARE " FROM " << mysqlpp::quote
				<< "SELECT " DB_TABLE_AVATAR_BLACKLIST_COL_ID " FROM " DB_TABLE_AVATAR_BLACKLIST " WHERE BINARY " DB_TABLE_AVATAR_BLACKLIST_COL_AVATAR_HASH " = ?";

		mysqlpp::Query prepareLogin = m_connData->conn.query();
		prepareLogin
				<< "PREPARE " QUERY_LOGIN_PREPARE " FROM " << mysqlpp::quote
				<< "UPDATE " DB_TABLE_PLAYER " SET " DB_TABLE_PLAYER_COL_LASTLOGIN " = ?, " DB_TABLE_PLAYER_COL_AVATARHASH " = ?, " DB_TABLE_PLAYER_COL_AVATARTYPE " = ? WHERE " DB_TABLE_PLAYER_COL_ID " = ?";
		mysqlpp::Query prepareCreateGame = m_connData->conn.query();
		prepareCreateGame
				<< "PREPARE " QUERY_CREATE_GAME_PREPARE " FROM " << mysqlpp::quote
				<< "INSERT INTO " DB_TABLE_GAME " (" DB_TABLE_GAME_COL_NAME ", " DB_TABLE_GAME_COL_STARTTIME ") VALUES (?, ?)";
		mysqlpp::Query prepareEndGame = m_connData->conn.query();
		prepareEndGame
				<< "PREPARE " QUERY_END_GAME_PREPARE " FROM " << mysqlpp::quote
				<< "UPDATE " DB_TABLE_GAME " SET " DB_TABLE_GAME_COL_ENDTIME " = ? WHERE " DB_TABLE_GAME_COL_ID " = ?";
		mysqlpp::Query prepareRelation = m_connData->conn.query();
		prepareRelation
				<< "PREPARE " QUERY_GAME_PLAYER_PREPARE " FROM " << mysqlpp::quote
				<< "INSERT INTO " DB_TABLE_GAMEPLAYER " (" DB_TABLE_GAMEPLAYER_COL_GAMEID ", " DB_TABLE_GAMEPLAYER_COL_PLAYERID ", " DB_TABLE_GAMEPLAYER_COL_PLACE ") VALUES (?, ?, ?)";
		mysqlpp::Query prepareScore = m_connData->conn.query();
		prepareScore
				<< "PREPARE " QUERY_UPDATE_SCORE_PREPARE " FROM " << mysqlpp::quote
				<< "CALL updatePointsForGame(?)";

		mysqlpp::Query prepareReportAvatar = m_connData->conn.query();
		prepareReportAvatar
				<< "PREPARE " QUERY_REPORT_AVATAR_PREPARE " FROM " << mysqlpp::quote
				<< "INSERT INTO " DB_TABLE_REP_AVATAR " (" DB_TABLE_REP_AVATAR_COL_PLAYERID ", " DB_TABLE_REP_AVATAR_COL_AVATARHASH ", " DB_TABLE_REP_AVATAR_COL_AVATARTYPE ", " DB_TABLE_REP_AVATAR_COL_BY_PLAYERID ", " DB_TABLE_REP_AVATAR_COL_TIMESTAMP ") VALUES (?, ?, ?, ?, ?)";

		mysqlpp::Query prepareReportGame = m_connData->conn.query();
		prepareReportGame
				<< "PREPARE " QUERY_REPORT_GAME_PREPARE " FROM " << mysqlpp::quote
				<< "INSERT INTO " DB_TABLE_REP_GAME " (" DB_TABLE_REP_GAME_COL_CREATOR ", " DB_TABLE_REP_GAME_COL_GAMENAME ", " DB_TABLE_REP_GAME_COL_BY_PLAYERID ", " DB_TABLE_REP_GAME_COL_TIMESTAMP ", " DB_TABLE_REP_GAME_COL_GAMEID ") VALUES (?, ?, ?, ?, ?)";

		mysqlpp::Query prepareAdminPlayer = m_connData->conn.query();
		prepareAdminPlayer
				<< "PREPARE " QUERY_ADMIN_PLAYER_PREPARE " FROM " << mysqlpp::quote
				<< "SELECT " DB_TABLE_ADMIN_PLAYER_COL_PLAYERID " FROM " DB_TABLE_ADMIN_PLAYER;

		mysqlpp::Query prepareBlockPlayer = m_connData->conn.query();
		prepareBlockPlayer
				<< "PREPARE " QUERY_BLOCK_PLAYER_PREPARE " FROM " << mysqlpp::quote
				<< "UPDATE " DB_TABLE_PLAYER " SET " DB_TABLE_PLAYER_COL_VALID " = ?, " DB_TABLE_PLAYER_COL_ACTIVE " = ? WHERE " DB_TABLE_PLAYER_COL_ID " = ?";
		mysqlpp::Query preparePlayerLastGames = m_connData->conn.query();
		preparePlayerLastGames
				<< "PREPARE " QUERY_PLAYER_LASTGAMES_PREPARE " FROM " << mysqlpp::quote
				<< "UPDATE " DB_TABLE_PLAYER " SET " DB_TABLE_PLAYER_COL_LASTGAMES " = ?, " DB_TABLE_PLAYER_COL_LASTIP " = ? WHERE " DB_TABLE_PLAYER_COL_ID " = ?";
		if (!prepareNick.exec() || !prepareAvatarBlacklist.exec() || !prepareLogin.exec() || !prepareCreateGame.exec()
				|| !prepareEndGame.exec() || !prepareRelation.exec() || !prepareScore.exec() || !prepareReportAvatar.exec()
				|| !prepareReportGame.exec() || !prepareAdminPlayer.exec() || !prepareBlockPlayer.exec() || !preparePlayerLastGames.exec()) {
			string tmpError = string(prepareNick.error()) + prepareAvatarBlacklist.error() + prepareLogin.error() + prepareCreateGame.error() +
							  prepareEndGame.error() + prepareRelation.error() + prepareScore.error() + prepareReportAvatar.error() +
							  prepareReportGame.error() + prepareAdminPlayer.error() + prepareBlockPlayer.error() + preparePlayerLastGames.error();
			m_connData->conn.disconnect();
			boost::asio::post(*m_ioService, boost::bind(&ServerDBCallback::ConnectFailed, &m_callback, tmpError));
			m_permanentError = true;
		} else {
			boost::asio::post(*m_ioService, boost::bind(&ServerDBCallback::ConnectSuccess, &m_callback));
			m_previouslyConnected = true;
		}
	}
}

// Helper: check whether a MySQL/Qt error message indicates a transient
// failure that is worth retrying (connection loss, deadlock, lock timeout).
static bool IsTransientDBError(const string &error)
{
	// Connection-level errors
	if (error.find("Lost connection") != string::npos)          return true;
	if (error.find("server has gone away") != string::npos)     return true;
	if (error.find("gone away") != string::npos)                return true;
	if (error.find("lost connection") != string::npos)          return true;
	// InnoDB deadlock / lock-wait timeout — safe to retry
	if (error.find("Deadlock") != string::npos)                 return true;
	if (error.find("deadlock") != string::npos)                 return true;
	if (error.find("Lock wait timeout") != string::npos)        return true;
	if (error.find("lock wait timeout") != string::npos)        return true;
	// Qt QMYSQL driver may report these
	if (error.find("QMYSQL: Unable to execute query") != string::npos
	    && (error.find("2006") != string::npos       // CR_SERVER_GONE_ERROR
	        || error.find("2013") != string::npos))  // CR_SERVER_LOST
		return true;
	return false;
}

// Helper: check whether the transient error is specifically a connection loss
// (as opposed to a deadlock that does not require reconnection).
static bool IsConnectionLossError(const string &error)
{
	if (error.find("Lost connection") != string::npos)          return true;
	if (error.find("server has gone away") != string::npos)     return true;
	if (error.find("gone away") != string::npos)                return true;
	if (error.find("lost connection") != string::npos)          return true;
	if (error.find("2006") != string::npos)                     return true;
	if (error.find("2013") != string::npos)                     return true;
	return false;
}

void
ServerDBThread::HandleNextQuery()
{
	boost::shared_ptr<AsyncDBQuery> nextQuery;
	{
		boost::mutex::scoped_lock lock(m_asyncQueueMutex);
		if (!m_asyncQueue.empty()) {
			nextQuery = m_asyncQueue.front();
			m_asyncQueue.pop();
		}
	}
	if (nextQuery) {
		bool queryFailed = false;
		// Number of immediate retries for transient errors (deadlock etc.)
		// that do NOT require a full reconnect.
		static const int MAX_TRANSIENT_RETRIES = 2;
		int transientRetries = 0;

		// Maximum number of times a query may be deferred (re-queued)
		// because a dependency (e.g. game DB ID from AsyncCreateGame)
		// is not yet available.  This allows the dependent CreateGame
		// query to be processed first.  If the limit is exceeded the
		// dependency will never arrive (permanent CreateGame failure)
		// and we give up with an error.
		static const unsigned MAX_DEFER_COUNT = 50;

retry_query:
		do {
			if (!nextQuery->Init(m_dbIdManager)) {
				// Dependency not available – defer this query.
				nextQuery->IncrementDeferCount();
				if (nextQuery->GetDeferCount() <= MAX_DEFER_COUNT) {
					LOG_ERROR("Deferring query " + nextQuery->GetPreparedName()
						+ " (defer #" + std::to_string(nextQuery->GetDeferCount())
						+ ") – waiting for dependency.");
					boost::mutex::scoped_lock lock(m_asyncQueueMutex);
					m_asyncQueue.push(nextQuery);
					m_semaphore.post();
				} else {
					LOG_ERROR("Query " + nextQuery->GetPreparedName()
						+ " deferred " + std::to_string(MAX_DEFER_COUNT)
						+ " times – dependency never arrived, giving up.");
					nextQuery->HandleError(*m_ioService, m_callback);
				}
				break;
			}
			mysqlpp::Query executeQuery = m_connData->conn.query();
			executeQuery << "EXECUTE " << nextQuery->GetPreparedName();

			list<string> paramList;
			nextQuery->GetParams(paramList);
			if (!paramList.empty()) {
				executeQuery << " using ";
				mysqlpp::Query paramQuery = m_connData->conn.query();
				paramQuery << "SET ";
				unsigned counter = 1;
				list<string>::iterator i = paramList.begin();
				list<string>::iterator end = paramList.end();
				while (i != end) {
					if (counter > 1) {
						paramQuery << ", ";
						executeQuery << ", ";
					}
					paramQuery << "@param" << counter << " = ";
					if (*i == "NULL") {
						paramQuery << "NULL";
					} else {
						paramQuery << "_utf8" << mysqlpp::quote << *i;
					}
					executeQuery << "@param" << counter;
					++counter;
					++i;
				}
				if (!paramQuery.exec()) {
					string tmpError = paramQuery.error();
					if (IsTransientDBError(tmpError)) {
						if (IsConnectionLossError(tmpError) || !m_connData->conn.connected()) {
							m_connData->conn.disconnect();
							queryFailed = true;
							break;
						}
						// Deadlock / lock-wait: retry in-place
						if (++transientRetries <= MAX_TRANSIENT_RETRIES) {
							LOG_ERROR("Transient DB error in param-set (retry "
								+ std::to_string(transientRetries) + "): " + tmpError);
							Msleep(50 * transientRetries);
							goto retry_query;
						}
					}
					m_connData->conn.disconnect();
					boost::asio::post(*m_ioService, boost::bind(&ServerDBCallback::QueryError, &m_callback, tmpError));
					queryFailed = true;
					break;
				}
			}
			if (nextQuery->RequiresResultSet()) {
				mysqlpp::StoreQueryResult res = executeQuery.store();
				if (res) {
					nextQuery->HandleResult(executeQuery, m_dbIdManager, res, *m_ioService, m_callback);
				} else {
					string error = executeQuery.error();
					if (IsTransientDBError(error)) {
						if (IsConnectionLossError(error) || !m_connData->conn.connected()) {
							m_connData->conn.disconnect();
							queryFailed = true;
							break;
						}
						if (++transientRetries <= MAX_TRANSIENT_RETRIES) {
							LOG_ERROR("Transient DB error in store (retry "
								+ std::to_string(transientRetries) + "): " + error);
							Msleep(50 * transientRetries);
							goto retry_query;
						}
					}
					nextQuery->HandleError(*m_ioService, m_callback);
				}
			} else {
				if (executeQuery.exec()) {
					nextQuery->HandleNoResult(executeQuery, m_dbIdManager, *m_ioService, m_callback);
				} else {
					string error = executeQuery.error();
					if (IsTransientDBError(error)) {
						if (IsConnectionLossError(error) || !m_connData->conn.connected()) {
							m_connData->conn.disconnect();
							queryFailed = true;
							break;
						}
						if (++transientRetries <= MAX_TRANSIENT_RETRIES) {
							LOG_ERROR("Transient DB error in exec (retry "
								+ std::to_string(transientRetries) + "): " + error);
							Msleep(50 * transientRetries);
							goto retry_query;
						}
					}
					nextQuery->HandleError(*m_ioService, m_callback);
				}
			}
		} while (nextQuery->Next()); // Consider composite queries.
		
		// If query failed due to connection loss, put it back in the queue
		// so it can be retried after reconnection
		if (queryFailed) {
			boost::mutex::scoped_lock lock(m_asyncQueueMutex);
			m_asyncQueue.push(nextQuery);
			// Post semaphore again so the query will be processed after reconnection
			m_semaphore.post();
		}
	}
}

void
ServerDBThread::SetConnected(bool isConnected)
{
	boost::mutex::scoped_lock lock(m_isConnectedMutex);
	m_isConnected = isConnected;
}

