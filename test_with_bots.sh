#!/bin/bash
# PokerTH Bot Test Script
# Startet headless Bot-Clients für realistische Tests

set -e

# Defaults
SERVER="${1:-localhost}"
PORT="${2:-7234}"
NUM_BOTS="${3:-10}"
START_ID="${4:-1}"
MODE="${5:-create}"  # create | join
GAME_ID="${6:-}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}PokerTH Bot Test Client${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""
echo "Configuration:"
echo "  Server:    $SERVER:$PORT"
echo "  Bots:      $NUM_BOTS (test$START_ID - test$((START_ID + NUM_BOTS - 1)))"
echo "  Mode:      $MODE"
if [ "$MODE" = "join" ] && [ -n "$GAME_ID" ]; then
    echo "  Game ID:   $GAME_ID"
fi
echo ""

# Prüfe ob Binary existiert
BOT_BIN="./build/bin/pokerth_bot"
if [ ! -f "$BOT_BIN" ]; then
    echo -e "${RED}Error: $BOT_BIN not found!${NC}"
    echo "Build it first with:"
    echo "  cd build && cmake .. && ninja pokerth_bot"
    exit 1
fi

# Baue Command
CMD="$BOT_BIN -s $SERVER -p $PORT -b $NUM_BOTS -i $START_ID"

if [ "$MODE" = "create" ]; then
    CMD="$CMD -c -g 'Bot Test Game $(date +%H:%M)'"
    echo -e "${YELLOW}Creating game with first bot, others will join...${NC}"
elif [ "$MODE" = "join" ] && [ -n "$GAME_ID" ]; then
    CMD="$CMD -j $GAME_ID"
    echo -e "${YELLOW}All bots joining existing game $GAME_ID...${NC}"
else
    echo -e "${YELLOW}Bots will login and wait in lobby...${NC}"
fi

echo ""
echo -e "${GREEN}Starting bots...${NC}"
echo "Command: $CMD"
echo ""
echo "Press Ctrl+C to stop all bots"
echo ""

# Starte Bots
$CMD
