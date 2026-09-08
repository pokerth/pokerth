--
-- PokerTH server live statistics
--
-- One row the official server overwrites once per statistics tick, so that
-- pokerth.net can show what is going on right now without opening a protobuf
-- client connection. `players_online` could also be counted from
-- `server_session`, but the number of open tables and the number of players
-- waiting in the lobby exist only in the running server's memory.
--
-- Apply to the same schema as docs/server_activity_schema.sql:
--
--   mysql -u root pokerth < docs/server_live_stats_schema.sql
--
-- If the table is missing the server logs that live statistics are off and
-- carries on - the statement is prepared outside the all-or-nothing check that
-- guards session logging, which in turn is outside the one that guards logins.
--

--
-- Exactly one row, `id` = 1, forever: every write is an upsert of that row, so
-- the table never grows. `run_id` is informational and says which process
-- wrote the snapshot; it is NULL if the run row could not be written.
--
-- `updated_at` is server-local wall clock, the same time base as
-- `server_session.connected_at` - deliberately not NOW(), whose time zone is
-- the database session's, not the server's. A reader decides "live" vs "stale"
-- from it: the server refreshes the row roughly every 45 seconds, so a row
-- older than a few minutes means the process is gone.
--
-- On a clean shutdown the server writes all three counts as 0 with a fresh
-- `updated_at`, which reads as "offline" immediately. After a crash nothing is
-- written and the row simply ages out.
--
CREATE TABLE IF NOT EXISTS `server_live_stats` (
  `id`              tinyint unsigned  NOT NULL,
  `run_id`          int unsigned      DEFAULT NULL COMMENT 'server_run.run_id of the writing process',
  `players_online`  smallint unsigned NOT NULL DEFAULT 0 COMMENT 'established sessions: lobby + in games',
  `tables_running`  smallint unsigned NOT NULL DEFAULT 0 COMMENT 'open games in the lobby game list',
  `players_waiting` smallint unsigned NOT NULL DEFAULT 0 COMMENT 'established sessions in the lobby, not seated at a game',
  `updated_at`      datetime          NOT NULL COMMENT 'server-local wall clock, same convention as server_session.connected_at',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Seed the singleton, dated so that it reads as stale until the server has
-- written its first heartbeat.
--
INSERT IGNORE INTO `server_live_stats` (`id`, `updated_at`) VALUES (1, '1970-01-01 00:00:00');

--
-- Reference queries.
--
-- The whole widget, in one row:
--
--   SELECT players_online, tables_running, players_waiting,
--          TIMESTAMPDIFF(SECOND, updated_at, NOW()) AS age_s
--     FROM server_live_stats WHERE id = 1;
--
-- Cross-check against the session log, which counts the same connections:
--
--   SELECT COUNT(*) FROM server_session
--    WHERE run_id = (SELECT MAX(run_id) FROM server_run)
--      AND disconnected_at IS NULL;
--
