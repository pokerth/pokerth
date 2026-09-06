--
-- PokerTH server activity logging - migration: client_platform
--
-- Adds the operating system a session logged in from. Until now only
-- `client_build_id` was stored, and its type byte names the client (Qt widget,
-- QML, web) rather than the platform - the QML client is the same build id on
-- Linux, Windows, macOS, Android and iOS, so mobile logins were not countable.
-- Clients from 2.1.9 on report the platform in their init message; older ones
-- do not, and their rows keep `client_platform` NULL.
--
-- Apply to the schema holding `server_session`:
--
--   mysql -u root pokerth < docs/server_activity_add_client_platform.sql
--
-- Safe to run twice: the ALTER is skipped if the column is already there.
--

SET @ddl := (
  SELECT IF(COUNT(*) > 0,
            'SELECT ''client_platform already present, nothing to do'' AS note',
            'ALTER TABLE `server_session`
               ADD COLUMN `client_platform` tinyint unsigned DEFAULT NULL
                    COMMENT ''1 = Windows, 2 = Linux, 3 = Mac, 4 = Android, 5 = iOS; NULL = not reported''
                    AFTER `client_build_id`,
               ADD KEY `client_platform` (`client_platform`)')
    FROM information_schema.COLUMNS
   WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME   = 'server_session'
     AND COLUMN_NAME  = 'client_platform');

PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

--
-- Reference queries for the new column.
--
-- Logins by platform over the last 30 days (rows before the upgrade, and
-- clients too old to report one, show up as NULL):
--
--   SELECT client_platform, COUNT(*) AS logins, COUNT(DISTINCT player_id) AS players
--     FROM server_session
--    WHERE connected_at >= CURDATE() - INTERVAL 30 DAY
--    GROUP BY client_platform ORDER BY logins DESC;
--
-- Mobile share of the QML client (client_type 2):
--
--   SELECT DATE(connected_at) AS day,
--          SUM(client_platform IN (4, 5)) AS mobile,
--          SUM(client_platform IN (1, 2, 3)) AS desktop
--     FROM server_session
--    WHERE client_type = 2 AND connected_at >= CURDATE() - INTERVAL 30 DAY
--    GROUP BY day ORDER BY day;
--
