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

#ifndef _DB_TABLE_DEFS_H_
#define _DB_TABLE_DEFS_H_

#define DB_TABLE_PLAYER								"player"
#define DB_TABLE_PLAYER_COL_ID						"player_id"
#define DB_TABLE_PLAYER_COL_USERNAME				"username"
#define DB_TABLE_PLAYER_COL_PASSWORD				"password"
#define DB_TABLE_PLAYER_COL_VALID					"blocked"
#define DB_TABLE_PLAYER_COL_COUNTRY					"country_iso"
#define DB_TABLE_PLAYER_COL_LASTLOGIN				"last_login"
#define DB_TABLE_PLAYER_COL_ACTIVE					"active"
#define DB_TABLE_PLAYER_COL_AVATARHASH				"avatar_hash"
#define DB_TABLE_PLAYER_COL_AVATARTYPE				"avatar_mime"
#define DB_TABLE_PLAYER_COL_LASTGAMES				"last_games"
#define DB_TABLE_PLAYER_COL_LASTIP				    "last_ip"

#define DB_TABLE_GAME								"game"
#define DB_TABLE_GAME_COL_ID						"idgame"
#define DB_TABLE_GAME_COL_NAME						"name"
#define DB_TABLE_GAME_COL_STARTTIME					"start_time"
#define DB_TABLE_GAME_COL_ENDTIME					"end_time"

#define DB_TABLE_GAMEPLAYER							"game_has_player"
#define DB_TABLE_GAMEPLAYER_COL_GAMEID				"game_idgame"
#define DB_TABLE_GAMEPLAYER_COL_PLAYERID			"player_idplayer"
#define DB_TABLE_GAMEPLAYER_COL_PLACE				"place"

#define DB_TABLE_REP_AVATAR							"reported_avatar"
#define DB_TABLE_REP_AVATAR_COL_PLAYERID			"idplayer"
#define DB_TABLE_REP_AVATAR_COL_AVATARHASH			"avatar_hash"
#define DB_TABLE_REP_AVATAR_COL_AVATARTYPE			"avatar_type"
#define DB_TABLE_REP_AVATAR_COL_BY_PLAYERID			"by_idplayer"
#define DB_TABLE_REP_AVATAR_COL_TIMESTAMP			"timestamp"

#define DB_TABLE_REP_GAME							"reported_gamename"
#define DB_TABLE_REP_GAME_COL_CREATOR				"game_creator_idplayer"
#define DB_TABLE_REP_GAME_COL_GAMEID				"game_idgame"
#define DB_TABLE_REP_GAME_COL_GAMENAME				"game_name"
#define DB_TABLE_REP_GAME_COL_BY_PLAYERID			"by_idplayer"
#define DB_TABLE_REP_GAME_COL_TIMESTAMP				"timestamp"

#define DB_TABLE_AVATAR_BLACKLIST					"avatar_blacklist"
#define DB_TABLE_AVATAR_BLACKLIST_COL_ID			"id"
#define DB_TABLE_AVATAR_BLACKLIST_COL_AVATAR_HASH	"avatar_hash"

#define DB_TABLE_ADMIN_PLAYER						"admin_player"
#define DB_TABLE_ADMIN_PLAYER_COL_PLAYERID			"admin_idplayer"

// Lobby activity logging, see docs/server_activity_schema.sql.
#define DB_TABLE_SERVER_RUN							"server_run"
#define DB_TABLE_SERVER_RUN_COL_ID					"run_id"
#define DB_TABLE_SERVER_RUN_COL_STARTED				"started_at"
#define DB_TABLE_SERVER_RUN_COL_STOPPED				"stopped_at"
#define DB_TABLE_SERVER_RUN_COL_BUILDID				"build_id"

#define DB_TABLE_SERVER_SESSION						"server_session"
#define DB_TABLE_SERVER_SESSION_COL_ID				"session_id"
#define DB_TABLE_SERVER_SESSION_COL_RUNID			"run_id"
#define DB_TABLE_SERVER_SESSION_COL_NO				"session_no"
#define DB_TABLE_SERVER_SESSION_COL_PLAYERID		"player_id"
#define DB_TABLE_SERVER_SESSION_COL_NICK			"nick"
#define DB_TABLE_SERVER_SESSION_COL_ISGUEST			"is_guest"
#define DB_TABLE_SERVER_SESSION_COL_BUILDID			"client_build_id"
#define DB_TABLE_SERVER_SESSION_COL_COUNTRY			"country_iso"
#define DB_TABLE_SERVER_SESSION_COL_IP				"ip"
#define DB_TABLE_SERVER_SESSION_COL_CONNECTED		"connected_at"
#define DB_TABLE_SERVER_SESSION_COL_DISCONNECTED	"disconnected_at"
#define DB_TABLE_SERVER_SESSION_COL_DURATION		"duration_s"
#define DB_TABLE_SERVER_SESSION_COL_GAMEID			"last_game_id"
#define DB_TABLE_SERVER_SESSION_COL_CLOSEREASON		"close_reason"

#endif // DB_TABLE_DEFS_H
