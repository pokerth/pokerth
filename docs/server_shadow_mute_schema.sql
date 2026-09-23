--
-- PokerTH shadow mute
--
-- A shadow muted player keeps chatting as usual and sees their own lines, but
-- the server delivers them to nobody else: not to the lobby, not to the game
-- table, not to Discord. Private messages are not affected. The player gets no
-- hint whatsoever - that is the whole point.
--
-- The mute is active while `shadow_mute_until` lies in the future, measured by
-- the database clock (NOW()). NULL means not muted; for an open-ended mute use
-- '9999-12-31 23:59:59'. The server re-reads the list every few seconds, so
-- setting, extending or lifting a mute takes effect without a re-login.
--
-- Apply to the same schema the server already uses (the one holding `player`):
--
--   mysql -u root pokerth < docs/server_shadow_mute_schema.sql
--
-- Without the column the server logs that shadow mute is off and carries on.
--

ALTER TABLE `player`
  ADD COLUMN `shadow_mute_until` DATETIME NULL DEFAULT NULL
    COMMENT 'Shadow mute active while > NOW(); NULL = not muted'
    AFTER `blocked`,
  ADD KEY `shadow_mute_until` (`shadow_mute_until`);

--
-- Examples:
--
--   UPDATE player SET shadow_mute_until = '2026-10-31 23:59:59' WHERE username = 'somebody';
--   UPDATE player SET shadow_mute_until = NOW() + INTERVAL 7 DAY  WHERE username = 'somebody';
--   UPDATE player SET shadow_mute_until = NULL                    WHERE username = 'somebody';
--
