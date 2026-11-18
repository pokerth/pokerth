--
-- PokerTH server message log
--
-- Every line the official server writes to server_messages.log, also stored in
-- the database, so that the log can be read and filtered on the web server
-- without shipping the file around.
--
-- Apply to the same schema as docs/server_activity_schema.sql:
--
--   mysql -u root pokerth < docs/server_log_schema.sql
--
-- If the table is missing the server logs that database logging is off and
-- carries on - server_messages.log is written either way, the table is an
-- additional copy, not a replacement. Only the official server writes it.
--

--
-- One row per log line. Lines are buffered in memory and written in batches
-- from the database thread, so a short database outage loses nothing (up to
-- 20000 buffered lines). `log_id` is the order they were logged in;
-- `logged_at` is the time the line was produced, not the time it reached the
-- database.
--
-- `logged_at` is server-local wall clock with second precision, the same time
-- stamp as in server_messages.log and the same convention as
-- `server_session.connected_at`. `run_id` is the server process, NULL only if
-- its server_run row could not be written.
--
-- `level` is the tag from the file: ERR for errors, MSG for regular messages,
-- OUT for the more verbose levels. The trailing newline of a file line is not
-- stored; bytes which are not valid UTF-8 are replaced by U+FFFD.
--
-- The remaining columns are taken from the message text when it contains them,
-- NULL otherwise - they exist for filtering and sorting, the message itself is
-- unchanged:
--
--   session_no  "session #12", "Session 12 (...)"; together with run_id the
--               key of server_session, which holds the account, client and
--               country of that connection
--   game_id     "game 41", "Game 41 -", "game=41"; the server's own game
--               number, restarts with every run like session_no
--   player_uid  "(id:382", "Player 382 offline ..."; the server's own player
--               number, restarts with every run
--   player_id   "dbId:33851"; player.player_id, NULL for guests
--   player_name the first quoted player name, e.g. Player "BigElz" or
--               player="BigElz"; "(unknown)" is stored as NULL
--   ip          the first IP address in the line, INET6_ATON form like
--               server_session.ip; IPv4-mapped IPv6 addresses are stored as
--               plain IPv4, so one filter finds both spellings
--
CREATE TABLE IF NOT EXISTS `server_log` (
  `log_id`      bigint unsigned NOT NULL AUTO_INCREMENT,
  `run_id`      int unsigned    DEFAULT NULL COMMENT 'server_run.run_id of the writing process',
  `logged_at`   datetime        NOT NULL COMMENT 'server-local wall clock, as in server_messages.log',
  `level`       enum('ERR','MSG','OUT') NOT NULL,
  `session_no`  int unsigned    DEFAULT NULL COMMENT 'server_session.session_no within run_id',
  `game_id`     int unsigned    DEFAULT NULL COMMENT 'server game number within run_id',
  `player_uid`  int unsigned    DEFAULT NULL COMMENT 'server player number within run_id',
  `player_id`   int(11)         DEFAULT NULL COMMENT 'player.player_id, NULL for guests',
  `player_name` varchar(64)     DEFAULT NULL,
  `ip`          varbinary(16)   DEFAULT NULL COMMENT 'INET6_ATON form, holds both v4 and v6',
  `message`     text            NOT NULL,
  PRIMARY KEY (`log_id`),
  KEY `logged_at` (`logged_at`),
  KEY `level_logged_at` (`level`,`logged_at`),
  KEY `run_session` (`run_id`,`session_no`),
  KEY `run_game` (`run_id`,`game_id`),
  KEY `run_player_uid` (`run_id`,`player_uid`),
  KEY `player_id_logged_at` (`player_id`,`logged_at`),
  KEY `player_name_logged_at` (`player_name`,`logged_at`),
  KEY `ip_logged_at` (`ip`,`logged_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Reference queries.
--
-- The last 200 lines, newest first:
--
--   SELECT logged_at, level, message FROM server_log
--    ORDER BY log_id DESC LIMIT 200;
--
-- Errors of the last 24 hours:
--
--   SELECT logged_at, message FROM server_log
--    WHERE level = 'ERR' AND logged_at >= NOW() - INTERVAL 1 DAY
--    ORDER BY log_id;
--
-- Everything about one player name, or one address:
--
--   SELECT logged_at, level, message FROM server_log
--    WHERE player_name = 'BigElz' ORDER BY log_id DESC LIMIT 500;
--
--   SELECT logged_at, level, message FROM server_log
--    WHERE ip = INET6_ATON('86.186.133.115') ORDER BY log_id DESC LIMIT 500;
--
-- The complete story of one connection, including the lines which only name
-- the session number:
--
--   SELECT l.logged_at, l.level, l.message
--     FROM server_log l
--     JOIN server_session s ON s.run_id = l.run_id AND s.session_no = l.session_no
--    WHERE s.session_id = 123456
--    ORDER BY l.log_id;
--
-- One game of the current run:
--
--   SELECT logged_at, level, message FROM server_log
--    WHERE run_id = (SELECT MAX(run_id) FROM server_run) AND game_id = 41
--    ORDER BY log_id;
--
-- Retention. The lines contain IP addresses and player names; drop old ones
-- from a cron, e.g.:
--
--   DELETE FROM server_log WHERE logged_at < CURDATE() - INTERVAL 90 DAY;
--
-- Volume: the current server_messages.log has ~5300 lines a day at ~110
-- bytes, so with the indexes roughly 2 MB a day.
--
