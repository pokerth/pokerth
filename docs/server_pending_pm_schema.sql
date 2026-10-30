--
-- PokerTH held back private messages
--
-- A private message to a player at a running table, or to a player who is
-- offline, is not delivered right away: the server keeps it here and hands it
-- over once the player is out of the game, or at the next login. Offline
-- players can only be written to from an existing conversation in the client.
--
-- The server reads the table once when it connects to the database and keeps
-- it in memory; every change is written back (INSERT when a message is held
-- back, DELETE when it is delivered or expired). Limits: 2 waiting messages
-- per sender and receiver, 20 per receiver, 20 per sender; a message expires
-- after 30 days.
--
-- Apply to the same schema the server already uses (the one holding `player`):
--
--   mysql -u root pokerth < docs/server_pending_pm_schema.sql
--
-- Without the table the server logs that held back private messages are off
-- and rejects private messages to players at a running table, as before.
--

CREATE TABLE IF NOT EXISTS `pending_private_message` (
  `message_id`   bigint unsigned NOT NULL AUTO_INCREMENT,
  `recipient_id` int(11)         NOT NULL COMMENT 'player.player_id of the receiver',
  `sender_id`    int(11)         NOT NULL COMMENT 'player.player_id of the sender',
  `text`         varchar(128)    NOT NULL COMMENT 'at most 128 bytes UTF-8, the chat limit',
  `created_at`   datetime        NOT NULL,
  PRIMARY KEY (`message_id`),
  KEY `recipient_id` (`recipient_id`),
  KEY `sender_id` (`sender_id`),
  KEY `created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
