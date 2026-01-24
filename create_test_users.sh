#!/bin/bash
# PokerTH - Test User Creation Script
# Erstellt test1 bis test100 in der Datenbank für Bot-Tests

# Konfiguration
DB_HOST="${1:-localhost}"
DB_NAME="${2:-pokerth_ranking_test}"
DB_USER="${3:-root}"
AES_KEY="${5}"  # REQUIRED: Pass as 5th argument
NUM_USERS="${4:-100}"

if [ -z "$AES_KEY" ]; then
    echo "ERROR: AES_KEY required as 5th argument!"
    echo "Usage: $0 <host> <db_name> <db_user> <num_users> <aes_key>"
    exit 1
fi

echo "=========================================="
echo "PokerTH Test User Creation"
echo "=========================================="
echo "Host:      $DB_HOST"
echo "Database:  $DB_NAME"
echo "User:      $DB_USER"
echo "Creating:  test1 - test$NUM_USERS"
echo ""

read -sp "MySQL Password: " DB_PASS
echo ""
echo ""

# SQL Script generieren
SQL_FILE="/tmp/create_pokerth_testusers.sql"
echo "-- PokerTH Test Users" > "$SQL_FILE"
echo "-- Generated: $(date)" >> "$SQL_FILE"
echo "" >> "$SQL_FILE"

for i in $(seq 1 $NUM_USERS); do
    USERNAME="test$i"
    PASSWORD="$USERNAME"  # Passwort = Username
    EMAIL="$USERNAME@pokerth.test"
    
    # Lösche existierenden User (falls vorhanden)
    echo "DELETE FROM player WHERE username = '$USERNAME';" >> "$SQL_FILE"
    echo "DELETE FROM player_ranking WHERE username = '$USERNAME';" >> "$SQL_FILE"
    
    # Insert in player Tabelle
    cat >> "$SQL_FILE" << EOF
INSERT INTO player (
    username, 
    password, 
    email, 
    created, 
    last_login,
    active, 
    blocked
) VALUES (
    '$USERNAME',
    AES_ENCRYPT('$PASSWORD', '$AES_KEY'),
    '$EMAIL',
    NOW(),
    NULL,
    1,
    0
);

EOF

    # Get player_id für player_ranking (via LAST_INSERT_ID)
    cat >> "$SQL_FILE" << EOF
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
    '$USERNAME',
    0,
    0,
    0
);

EOF
done

echo "SQL script generated: $SQL_FILE"
echo ""
echo "Executing SQL..."

# SQL ausführen
mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" < "$SQL_FILE"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Successfully created $NUM_USERS test users!"
    echo ""
    echo "Verification (showing first 10 test* users):"
    mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" -e \
        "SELECT username, email, created, active FROM player WHERE username REGEXP '^test[0-9]+$' ORDER BY CAST(SUBSTRING(username, 5) AS UNSIGNED) LIMIT 10;"
    echo ""
    echo "Total test[0-9]+ users:"
    mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" -e \
        "SELECT COUNT(*) as count FROM player WHERE username REGEXP '^test[0-9]+$';"
else
    echo ""
    echo "✗ Error creating test users!"
    exit 1
fi

# Cleanup
rm -f "$SQL_FILE"

echo ""
echo "Test users ready!"
echo "Bots can now use: test1/test1, test2/test2, ..., test$NUM_USERS/test$NUM_USERS"
