-- PokerTH Test Users - Manual SQL Template
-- Verwendung: Passe die Anzahl an und führe in MySQL aus

-- AES Key - REPLACE WITH YOUR KEY!
SET @aes_key = 'YOUR_AES_KEY_HERE';

-- Beispiel: test1 erstellen
DELETE FROM player WHERE username = 'test1';
DELETE FROM player_ranking WHERE username = 'test1';

INSERT INTO player (
    username, 
    password, 
    email, 
    created, 
    active, 
    blocked
) VALUES (
    'test1',
    AES_ENCRYPT('test1', @aes_key),
    'test1@pokerth.test',
    NOW(),
    1,
    0
);

INSERT INTO player_ranking (
    player_id,
    final_score,
    username,
    points_sum,
    season_games,
    average_score
) VALUES (
    LAST_INSERT_ID(),
    -1,
    'test1',
    0,
    0,
    0
);

-- Wiederholen für test2, test3, etc.
-- Oder Bash-Script verwenden: ./create_test_users.sh

-- Verification Query
SELECT 
    p.player_id,
    p.username,
    AES_DECRYPT(p.password, @aes_key) as decrypted_password,
    p.email,
    p.active,
    pr.season_games,
    pr.points_sum
FROM player p
LEFT JOIN player_ranking pr ON p.player_id = pr.player_id
WHERE p.username LIKE 'test%'
ORDER BY p.player_id
LIMIT 10;
