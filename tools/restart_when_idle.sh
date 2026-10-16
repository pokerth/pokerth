#!/usr/bin/env bash
#
# Waits until the PokerTH game server is idle, restarts the docker compose
# project once and exits.
#
# Idle means: no players online, or fewer than MIN_PLAYERS players online and
# no open tables. The numbers come from server_statistics.log, which the server
# rewrites at most every 45 s and only when a value changed.
#
# Usage: restart_when_idle.sh [--test]
#
#   --test        use the test server in /opt/pokerth_test_server
#
# Environment overrides:
#   COMPOSE_DIR   directory of the compose project (default: /opt/pokerth_server,
#                 with --test: /opt/pokerth_test_server)
#   STATS_FILE    path to server_statistics.log on the host
#   MIN_PLAYERS   below this many players a restart is allowed if no table is open (default: 5)
#   POLL_SECS     seconds between checks (default: 30)
#   STABLE_SECS   how long the idle condition must hold before restarting (default: 60)

set -u

DEFAULT_COMPOSE_DIR=/opt/pokerth_server
case "${1:-}" in
	"") ;;
	--test) DEFAULT_COMPOSE_DIR=/opt/pokerth_test_server ;;
	*)
		echo "Usage: $0 [--test]" >&2
		exit 2
		;;
esac

COMPOSE_DIR="${COMPOSE_DIR:-$DEFAULT_COMPOSE_DIR}"
STATS_FILE="${STATS_FILE:-$COMPOSE_DIR/data/server/log-files/server_statistics.log}"
MIN_PLAYERS="${MIN_PLAYERS:-5}"
POLL_SECS="${POLL_SECS:-30}"
STABLE_SECS="${STABLE_SECS:-60}"

log() {
	echo "$(date '+%Y-%m-%d %H:%M:%S') $*"
}

compose() {
	(cd "$COMPOSE_DIR" && docker compose "$@")
}

stat_value() {
	awk -v key="$1" '$1 == key { print $2; exit }' "$STATS_FILE" 2>/dev/null
}

is_number() {
	[[ "$1" =~ ^[0-9]+$ ]]
}

# Start time (epoch) of the earliest started running container of the project.
# The earliest one is the safe choice: a file older than that is certainly
# older than the game server's start.
project_started_at() {
	local id ts earliest=""
	for id in $(compose ps -q --status running); do
		ts=$(date -d "$(docker inspect -f '{{.State.StartedAt}}' "$id")" +%s) || continue
		if [ -z "$earliest" ] || [ "$ts" -lt "$earliest" ]; then
			earliest=$ts
		fi
	done
	echo "$earliest"
}

if ! compose config -q >/dev/null 2>&1; then
	log "No usable docker compose project in $COMPOSE_DIR."
	exit 1
fi

log "Waiting for idle server (compose dir=$COMPOSE_DIR, min_players=$MIN_PLAYERS, stable=${STABLE_SECS}s)."

idle_since=0
last_state=""

while true; do
	started_at=$(project_started_at)
	if [ -z "$started_at" ]; then
		log "No running containers in $COMPOSE_DIR - nothing to restart."
		exit 1
	fi

	if [ ! -r "$STATS_FILE" ]; then
		log "Cannot read $STATS_FILE - retrying."
		idle_since=0
		sleep "$POLL_SECS"
		continue
	fi

	file_mtime=$(stat -c %Y "$STATS_FILE")

	if [ "$file_mtime" -lt "$started_at" ]; then
		# The server writes the file only after a value changed. Not written
		# since the containers started means nobody has logged in yet, the
		# file still holds the numbers of the previous run.
		players=0
		games=0
	else
		players=$(stat_value CurPlayersLoggedIn)
		games=$(stat_value CurGamesOpen)
		if ! is_number "$players" || ! is_number "$games"; then
			log "Unexpected content in $STATS_FILE - retrying."
			idle_since=0
			sleep "$POLL_SECS"
			continue
		fi
	fi

	state="players=$players games=$games"
	if [ "$state" != "$last_state" ]; then
		log "$state"
		last_state="$state"
	fi

	if [ "$players" -eq 0 ] || { [ "$players" -lt "$MIN_PLAYERS" ] && [ "$games" -eq 0 ]; }; then
		now=$(date +%s)
		if [ "$idle_since" -eq 0 ]; then
			idle_since=$now
		elif [ $((now - idle_since)) -ge "$STABLE_SECS" ]; then
			log "Server idle ($state) - running docker compose restart in $COMPOSE_DIR."
			if compose restart; then
				log "Restart done."
				exit 0
			fi
			log "Restart failed."
			exit 1
		fi
	else
		idle_since=0
	fi

	sleep "$POLL_SECS"
done
