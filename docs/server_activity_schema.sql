--
-- PokerTH server activity logging
--
-- Two tables the official server writes to directly, so that lobby activity is
-- queryable instead of only existing as text in server_messages.log.
--
-- Apply to the same schema the server already uses (the one holding `player`,
-- `game` and `game_has_player`):
--
--   mysql -u root pokerth < docs/server_activity_schema.sql
--
-- If either table is missing, the server logs that activity logging is off and
-- carries on: preparing these two statements is deliberately kept out of the
-- all-or-nothing check that guards the statements logins depend on.
--

--
-- One row per server process. Without it a gap in the session data is
-- ambiguous - it could mean "nobody played" or "the server was down", and the
-- two look identical in a chart. It also disambiguates `session_no`, which the
-- server restarts from scratch on every launch.
--
CREATE TABLE IF NOT EXISTS `server_run` (
  `run_id`     int unsigned NOT NULL AUTO_INCREMENT,
  `started_at` datetime     NOT NULL,
  `stopped_at` datetime     DEFAULT NULL COMMENT 'NULL while running, or if the process was killed',
  `build_id`   int unsigned DEFAULT NULL COMMENT 'server build id, same encoding as the client one',
  PRIMARY KEY (`run_id`),
  KEY `started_at` (`started_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- One row per connection that reached the lobby, written on connect and
-- completed on disconnect. Guests are included - they are a real part of the
-- traffic - and are told apart by `is_guest`, since they have no `player_id`.
--
-- Sessions still open have `disconnected_at IS NULL`; that is also how you ask
-- who is online right now. Rows left open by a crashed server are marked
-- `close_reason = 'server_gone'` on the next start, so they can be excluded
-- without pretending to know when they ended.
--
CREATE TABLE IF NOT EXISTS `server_session` (
  `session_id`      bigint unsigned NOT NULL AUTO_INCREMENT,
  `run_id`          int unsigned    NOT NULL,
  `session_no`      int unsigned    NOT NULL COMMENT 'the #N from server_messages.log, unique within a run',
  `player_id`       int(11)         DEFAULT NULL COMMENT 'player.player_id, NULL for guests',
  `nick`            varchar(64)     NOT NULL COMMENT 'name used for this session, kept for guests and renames',
  `is_guest`        tinyint(1)      NOT NULL DEFAULT 0,
  `client_build_id` int unsigned    DEFAULT NULL COMMENT '(type << 24) | (major << 16) | (minor << 8) | revision',
  `client_type`     tinyint unsigned GENERATED ALWAYS AS (`client_build_id` >> 24) VIRTUAL
                    COMMENT '1 = Qt widget, 2 = QML - derived, never stored twice',
  `country_iso`     varchar(32)     DEFAULT NULL,
  `ip`              varbinary(16)   DEFAULT NULL COMMENT 'INET6_ATON form, holds both v4 and v6',
  `connected_at`    datetime        NOT NULL,
  `disconnected_at` datetime        DEFAULT NULL COMMENT 'NULL while the session is open',
  `duration_s`      int unsigned    DEFAULT NULL,
  `last_game_id`    int(11)         DEFAULT NULL COMMENT 'game the session was in when it ended',
  `close_reason`    varchar(48)     DEFAULT NULL COMMENT 'asio/system error, or server_gone',
  PRIMARY KEY (`session_id`),
  UNIQUE KEY `run_session` (`run_id`,`session_no`),
  KEY `connected_at` (`connected_at`),
  KEY `player_connected` (`player_id`,`connected_at`),
  KEY `ip_connected` (`ip`,`connected_at`),
  KEY `client_type` (`client_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Reference queries - these are what the website charts are built on.
--
-- Daily active registered accounts:
--
--   SELECT DATE(connected_at) AS day, COUNT(DISTINCT player_id) AS players
--     FROM server_session
--    WHERE is_guest = 0 AND player_id IS NOT NULL
--    GROUP BY day ORDER BY day;
--
-- New registrations per day come from the player table itself and need no log:
--
--   SELECT DATE(created) AS day, COUNT(*) AS registrations
--     FROM player GROUP BY day ORDER BY day;
--
-- Return rate of accounts registered in the last 30 days (7 day window):
--
--   SELECT COUNT(*) AS newcomers,
--          SUM(back_within_7d) AS returned
--     FROM (SELECT p.player_id,
--                  MAX(DATE(s.connected_at) > DATE(p.created)
--                      AND s.connected_at < p.created + INTERVAL 8 DAY) AS back_within_7d
--             FROM player p
--             JOIN server_session s ON s.player_id = p.player_id
--            WHERE p.created >= CURDATE() - INTERVAL 30 DAY
--              AND p.created <  CURDATE() - INTERVAL 7 DAY
--            GROUP BY p.player_id) t;
--
-- Logins by hour of day, for the quiet-hours view:
--
--   SELECT HOUR(connected_at) AS hour, COUNT(*) AS logins
--     FROM server_session
--    WHERE connected_at >= CURDATE() - INTERVAL 30 DAY
--    GROUP BY hour ORDER BY hour;
--
-- Client version adoption:
--
--   SELECT client_type,
--          client_build_id >> 16 & 0xFF AS major,
--          client_build_id >>  8 & 0xFF AS minor,
--          client_build_id       & 0xFF AS revision,
--          COUNT(DISTINCT player_id) AS players
--     FROM server_session
--    WHERE connected_at >= CURDATE() - INTERVAL 7 DAY
--    GROUP BY client_build_id ORDER BY players DESC;
--
-- Who is online right now:
--
--   SELECT nick, connected_at FROM server_session
--    WHERE disconnected_at IS NULL AND close_reason IS NULL;
--

--
-- Retention. The IP address is the only column here that identifies a person
-- beyond what `player` already stores, and it is kept for abuse handling, not
-- for the charts - none of the queries above touch it. Drop it once it has
-- served that purpose, e.g. from a monthly cron:
--
--   UPDATE server_session SET ip = NULL
--    WHERE ip IS NOT NULL AND connected_at < CURDATE() - INTERVAL 90 DAY;
--
-- The rows themselves are small (~100 bytes); at the current ~850 sessions a
-- day that is roughly 30 MB a year, so they can simply be kept.
--
