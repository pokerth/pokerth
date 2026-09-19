#!/bin/bash
#
# protonvpn-block.sh - block ProtonVPN nodes from the PokerTH game ports.
#
# Downloads the current ProtonVPN address lists, puts them into an ipset and
# rejects TCP connections from those addresses on the game ports (7234 plain,
# 7236 TLS). IPv4 only - the server is not reached over IPv6.
#
# Why not the Proton API directly: /vpn/logicals needs a logged-in account
# ("Access token does not have sufficient scope"), an anonymous session is not
# enough. The default sources below are lists that do that scraping daily.
#
# Why ranges and not single addresses: the exit IP the Proton API advertises
# is not the address a server actually sees - the real NAT egress address is a
# different one. Blocking the /24 is what catches most of that traffic; set
# USE_EXACT_IPS=1 for the narrow, largely ineffective variant. The egress can
# even sit in a neighbouring /24, which is what EXTRA_LIST below is for.
#
# The list is pulled once a day from cron. The last good list is cached on
# disk, so a failing download never opens the door: the previous set stays
# active, and "--from-cache" restores it after a reboot.
#
# Install (as root):
#   cp protonvpn-block.sh /usr/local/sbin/protonvpn-block
#   chmod 755 /usr/local/sbin/protonvpn-block
#   /usr/local/sbin/protonvpn-block --install-cron
#   /usr/local/sbin/protonvpn-block            # first run
#
# Requires: curl, ipset, iptables, flock.
#
# Usage:
#   protonvpn-block               update from the sources and apply the rules
#   protonvpn-block --from-cache  apply the cached list (used on @reboot)
#   protonvpn-block --dry-run     download and report only, change nothing
#                                 (no root needed; DUMP_TO=file writes the list)
#   protonvpn-block --status      show rules, set size and cache age
#   protonvpn-block --test-add IP add one address to the set, to verify the
#   protonvpn-block --test-del IP block from a machine you control
#   protonvpn-block --install-cron  write /etc/cron.d/protonvpn-block
#   protonvpn-block --remove      remove all rules and sets
#
# Addresses seen in the server log but missing from the sources go into
# /etc/protonvpn-block.extra (one IP or CIDR per line); they are merged into
# every run.
#

set -euo pipefail

# --- configuration ----------------------------------------------------------
# Override in /etc/default/protonvpn-block instead of editing this file.

# TCP ports to protect (iptables multiport list, max 15 entries).
PORTS="${PORTS:-7234,7236}"

# REJECT gives the client an immediate "connection refused", DROP lets it run
# into a timeout. REJECT is the friendlier and more honest answer.
ACTION="${ACTION:-REJECT}"

# Where the addresses come from. Plain text, one IP or CIDR per line, "#"
# comments allowed. Every source is downloaded and the results are merged, so
# a single dead URL does not empty the list.
#
#   tn3w/ProtonVPN-IPs  - Proton's own server list, scraped daily by a GitHub
#                         Action; protonvpn_ip_ranges.txt holds the /24 blocks
#   ipverse/asn-ip      - the prefixes Proton AG announces under AS62371,
#                         an independent second source
#
# Only exit addresses are used: a player's packets always arrive from the exit
# side, never from an entry address, and the entry ranges reach into /22
# blocks of shared hosters where unrelated customers would be caught.
SOURCES="${SOURCES:-
https://raw.githubusercontent.com/tn3w/ProtonVPN-IPs/master/protonvpn_ip_ranges.txt
https://raw.githubusercontent.com/ipverse/asn-ip/master/as/62371/ipv4-aggregated.txt
}"

# Use the exact advertised addresses instead of the /24 blocks. Narrower, but
# it misses the NAT egress addresses players actually arrive from.
USE_EXACT_IPS="${USE_EXACT_IPS:-0}"
SOURCES_EXACT="${SOURCES_EXACT:-
https://raw.githubusercontent.com/tn3w/ProtonVPN-IPs/master/protonvpn_ips.txt
}"

# Extra entries maintained by hand, one IP or CIDR per line, "#" comments
# allowed. They are merged into every run, including --from-cache, and survive
# the daily update. This is where an address belongs that was observed in the
# server log but is missing from the sources.
EXTRA_LIST="${EXTRA_LIST:-/etc/protonvpn-block.extra}"

# Refuse a list shorter than this - a truncated or mangled download must not
# silently shrink the block list.
MIN_ENTRIES="${MIN_ENTRIES:-100}"

# Refuse prefixes wider than this. The real lists stop at /18; anything wider
# would take a large chunk of the internet down with it.
MIN_PREFIX="${MIN_PREFIX:-18}"

# Never block these (space separated). The host's own global addresses and the
# address of an active SSH session are added automatically; if one of them
# ends up inside a downloaded range, the update is refused instead of locking
# anyone out.
EXCLUDE_IPS="${EXCLUDE_IPS:-}"

STATE_DIR="${STATE_DIR:-/var/lib/protonvpn-block}"
LOCK_FILE="${LOCK_FILE:-/run/protonvpn-block.lock}"

[ -r /etc/default/protonvpn-block ] && . /etc/default/protonvpn-block

WORKDIR=""

SET="protonvpn4"
CHAIN="PROTONVPN"
TAG="protonvpn-block"

# --- helpers ----------------------------------------------------------------

# Cron mails whatever a job prints, so routine progress goes to syslog only
# and the terminal gets it when a human runs the script by hand. Errors always
# go to stderr, so a broken daily run does produce a mail.
log()  { logger -t "$TAG" -- "$*" 2>/dev/null || true; [ -t 1 ] && echo "$(date '+%F %T') $*" || true; }
warn() { logger -t "$TAG" -p user.warning -- "$*" 2>/dev/null || true; echo "$(date '+%F %T') WARNING: $*" >&2; }
die()  { logger -t "$TAG" -p user.err -- "$*" 2>/dev/null || true; echo "$(date '+%F %T') ERROR: $*" >&2; exit 1; }

cleanup() {
	[ -n "$WORKDIR" ] && rm -rf "$WORKDIR"
	return 0
}

workdir() {
	[ -n "$WORKDIR" ] || WORKDIR="$(mktemp -d)"
	echo "$WORKDIR"
}

need_root() {
	[ "$(id -u)" -eq 0 ] || die "must run as root"
}

need_tools() {
	local t
	for t in "$@"; do
		command -v "$t" >/dev/null 2>&1 || die "missing required tool: $t"
	done
}

# --- fetching ---------------------------------------------------------------

# Download every source into one file. A source that fails is a warning, not a
# failure: the remaining ones still produce a usable list, and the size checks
# below decide whether the result is good enough to apply.
fetch_sources() {
	local out="$1" urls="$2" tmp url code ok=0 total=0

	tmp="$(workdir)"
	: > "$out"

	for url in $urls; do
		total=$((total + 1))
		code="$(curl -sSL --max-time 60 --retry 2 --retry-delay 5 \
			-A "PokerTH/2.0 (Qt Network)" \
			-o "$tmp/src" -w '%{http_code}' "$url" || echo 000)"
		if [ "$code" != "200" ] || [ ! -s "$tmp/src" ]; then
			warn "source failed (HTTP $code): $url"
			continue
		fi
		cat "$tmp/src" >> "$out"
		ok=$((ok + 1))
	done

	[ "$ok" -gt 0 ] || return 1
	[ "$ok" -eq "$total" ] || warn "only $ok of $total sources could be downloaded"
	return 0
}

# --- parsing ----------------------------------------------------------------

# Strip comments and blank lines. An empty result is legitimate here, so the
# grep status must not escape - with pipefail it would abort the script.
strip_comments() {
	sed -e 's/#.*//' -e 's/[[:space:]]//g' | grep -v '^$' || true
}

# Public unicast IPv4 only, no prefix wider than MIN_PREFIX. "No match" is a
# valid outcome, hence the trailing || true.
filter_ips() {
	grep -Ex '([0-9]{1,3}\.){3}[0-9]{1,3}(/[0-9]{1,2})?' \
	| grep -Ev '^(0\.|10\.|127\.|169\.254\.|172\.(1[6-9]|2[0-9]|3[01])\.|192\.168\.|22[4-9]\.|2[3-5][0-9]\.)' \
	| awk -F/ -v min="$MIN_PREFIX" 'NF==1 || $2+0 >= min' \
	| exclude_listed | sort -Vu || true
}

exclude_listed() {
	if [ -z "${EXCLUDE_IPS// /}" ]; then
		cat
	else
		grep -Fxv -f <(printf '%s\n' $EXCLUDE_IPS) || true
	fi
}

# Merge the hand-maintained extra entries. Same filters as the downloaded
# sources, so a typo there cannot widen the block either.
merge_extra() {
	local list="$1" tmp before

	[ -r "$EXTRA_LIST" ] || return 0
	tmp="$(workdir)"
	strip_comments < "$EXTRA_LIST" | filter_ips > "$tmp/extra"
	[ -s "$tmp/extra" ] || return 0

	before="$(wc -l < "$list")"
	sort -Vu "$list" "$tmp/extra" > "$tmp/merged"
	cat "$tmp/merged" > "$list"
	log "$(( $(wc -l < "$list") - before )) extra entries merged from $EXTRA_LIST"
	return 0
}

# Addresses that must never be caught by the block: this host's own global
# addresses and the peer of an interactive SSH session.
protected_addresses() {
	printf '%s\n' ${EXCLUDE_IPS:-}
	ip -o -4 addr show scope global 2>/dev/null | awk '{print $4}' | cut -d/ -f1
	[ -n "${SSH_CLIENT:-}" ] && echo "${SSH_CLIENT%% *}"
	return 0
}

# --- firewall ---------------------------------------------------------------

# Replace the contents of the ipset atomically: build a temporary set, fill it,
# check it, swap it in. Readers (the running iptables rule) never see an empty
# or half-filled set.
load_set() {
	local file="$1" tmpset="${SET}_tmp" restore addr

	ipset destroy "$tmpset" 2>/dev/null || true
	ipset create "$SET" hash:net family inet hashsize 4096 maxelem 262144 -exist
	ipset create "$tmpset" hash:net family inet hashsize 4096 maxelem 262144

	restore="$(mktemp)"
	# -exist: a duplicate from a second source must not abort the restore.
	sed "s/^/add $tmpset /; s/\$/ -exist/" "$file" > "$restore"
	if ! ipset restore < "$restore"; then
		rm -f "$restore"
		ipset destroy "$tmpset" 2>/dev/null || true
		die "ipset restore failed"
	fi
	rm -f "$restore"

	# Locking ourselves out is the one failure mode that cannot be fixed
	# remotely, so check before the swap, not after.
	for addr in $(protected_addresses); do
		if ipset test "$tmpset" "$addr" 2>/dev/null; then
			ipset destroy "$tmpset"
			die "refusing to apply: $addr (this host or your SSH peer) is inside the downloaded list"
		fi
	done

	ipset swap "$tmpset" "$SET"
	ipset destroy "$tmpset"
}

# Put the set match into our own chain and hook that chain into the paths the
# game traffic actually takes. Every step is idempotent, so a daily run does
# not pile up rules.
ensure_rules() {
	local port

	iptables -L "$CHAIN" -n >/dev/null 2>&1 || iptables -N "$CHAIN"

	# The chain itself only decides by source address; which traffic reaches
	# it is the hooks' business below.
	local -a rule=(-m set --match-set "$SET" src)
	if [ "$ACTION" = "REJECT" ]; then
		# tcp-reset is what makes the client say "connection refused" right
		# away; an ICMP answer is dropped by many networks on the way back.
		rule+=(-p tcp -j REJECT --reject-with tcp-reset)
	else
		rule+=(-j DROP)
	fi
	# Insert first, then delete everything below it. That leaves exactly one
	# rule without ever opening a gap, and it clears out rules an earlier
	# version of this script put here (a plain -C/-A check would keep them,
	# and the stale one would shadow the current rule).
	iptables -I "$CHAIN" 1 "${rule[@]}"
	while iptables -D "$CHAIN" 2 2>/dev/null; do :; done

	# A server running on the host is reached through INPUT, where the
	# destination port is still the port the client asked for.
	iptables -C INPUT -p tcp -m multiport --dports "$PORTS" -j "$CHAIN" 2>/dev/null \
		|| iptables -I INPUT 1 -p tcp -m multiport --dports "$PORTS" -j "$CHAIN"

	# A server in a container is reached through DNAT and passes FORWARD, not
	# INPUT; DOCKER-USER is the chain Docker leaves for us and is traversed
	# before its own accept rules. By then the packet carries the container's
	# address and port, so match the port the client originally connected to
	# instead of the DNATed one - that works whatever the port mapping is.
	# On a fresh boot the @reboot run can come before dockerd has created
	# DOCKER-USER; creating it ourselves is safe, Docker picks up an existing
	# chain and jumps to it from FORWARD once it starts.
	if ! iptables -L DOCKER-USER -n >/dev/null 2>&1 \
	   && { command -v dockerd >/dev/null 2>&1 || command -v docker >/dev/null 2>&1; }; then
		iptables -N DOCKER-USER 2>/dev/null || true
	fi
	if iptables -L DOCKER-USER -n >/dev/null 2>&1; then
		for port in ${PORTS//,/ }; do
			iptables -C DOCKER-USER -p tcp -m conntrack --ctorigdstport "$port" -j "$CHAIN" 2>/dev/null \
				|| iptables -I DOCKER-USER 1 -p tcp -m conntrack --ctorigdstport "$port" -j "$CHAIN"
		done
	fi
	return 0
}

remove_rules() {
	local port

	while iptables -C INPUT -p tcp -m multiport --dports "$PORTS" -j "$CHAIN" 2>/dev/null; do
		iptables -D INPUT -p tcp -m multiport --dports "$PORTS" -j "$CHAIN"
	done
	if iptables -L DOCKER-USER -n >/dev/null 2>&1; then
		for port in ${PORTS//,/ }; do
			while iptables -C DOCKER-USER -p tcp -m conntrack --ctorigdstport "$port" -j "$CHAIN" 2>/dev/null; do
				iptables -D DOCKER-USER -p tcp -m conntrack --ctorigdstport "$port" -j "$CHAIN"
			done
		done
	fi
	if iptables -L "$CHAIN" -n >/dev/null 2>&1; then
		iptables -F "$CHAIN"
		iptables -X "$CHAIN"
	fi
}

# Earlier versions of this script also built an IPv6 set and ip6tables rules.
# The server is not reached over IPv6, so clear those out wherever they are
# still installed instead of leaving them behind.
remove_ipv6_leftovers() {
	local port
	command -v ip6tables >/dev/null 2>&1 || return 0

	while ip6tables -C INPUT -p tcp -m multiport --dports "$PORTS" -j "$CHAIN" 2>/dev/null; do
		ip6tables -D INPUT -p tcp -m multiport --dports "$PORTS" -j "$CHAIN"
	done
	if ip6tables -L DOCKER-USER -n >/dev/null 2>&1; then
		for port in ${PORTS//,/ }; do
			while ip6tables -C DOCKER-USER -p tcp -m conntrack --ctorigdstport "$port" -j "$CHAIN" 2>/dev/null; do
				ip6tables -D DOCKER-USER -p tcp -m conntrack --ctorigdstport "$port" -j "$CHAIN"
			done
		done
	fi
	if ip6tables -L "$CHAIN" -n >/dev/null 2>&1; then
		ip6tables -F "$CHAIN"
		ip6tables -X "$CHAIN"
	fi
	ipset destroy protonvpn6 2>/dev/null || true
	rm -f "$STATE_DIR/ips.v6"
	return 0
}

apply_list() {
	local list="$1"

	load_set "$list"
	ensure_rules
	remove_ipv6_leftovers

	log "active: $(wc -l < "$list") ProtonVPN entries rejected on ports $PORTS"
}

# --- commands ---------------------------------------------------------------

cmd_update() {
	local dry="${1:-0}" tmp raw list n urls

	tmp="$(workdir)"
	raw="$tmp/raw"
	list="$tmp/ips"

	if [ "$USE_EXACT_IPS" = "1" ]; then
		urls="$SOURCES_EXACT"
	else
		urls="$SOURCES"
	fi

	fetch_sources "$raw" "$urls" || die "no source could be downloaded, keeping the previous list"

	strip_comments < "$raw" | filter_ips > "$list"
	merge_extra "$list"

	n="$(wc -l < "$list")"
	[ "$n" -ge "$MIN_ENTRIES" ] || die "only $n entries after filtering (expected >= $MIN_ENTRIES), keeping the previous list"

	if [ "$dry" = "1" ]; then
		echo "dry run: $n ProtonVPN entries found, nothing changed"
		echo "would reject them on TCP ports $PORTS ($ACTION)"
		echo "sample:"
		head -n 5 "$list" | sed 's/^/  /'
		if [ -n "${DUMP_TO:-}" ]; then
			cat "$list" > "$DUMP_TO"
			echo "full list written to $DUMP_TO"
		fi
		return 0
	fi

	mkdir -p "$STATE_DIR"
	install -m 0644 "$list" "$STATE_DIR/ips.v4"

	apply_list "$list"
}

cmd_from_cache() {
	local tmp

	[ -s "$STATE_DIR/ips.v4" ] || die "no cached list in $STATE_DIR, run without --from-cache first"

	# Work on a copy so the cache keeps holding what was downloaded, and pick
	# up extra entries added since the last update.
	tmp="$(workdir)"
	cp "$STATE_DIR/ips.v4" "$tmp/cached"
	merge_extra "$tmp/cached"

	apply_list "$tmp/cached"
}

cmd_status() {
	if ipset list "$SET" -t >/dev/null 2>&1; then
		echo "$SET: $(ipset list "$SET" -t | sed -n 's/^Number of entries: //p') entries"
	else
		echo "$SET: not present"
	fi
	if [ -f "$STATE_DIR/ips.v4" ]; then
		echo "cache: $(wc -l < "$STATE_DIR/ips.v4") entries, updated $(date -r "$STATE_DIR/ips.v4" '+%F %T')"
	else
		echo "cache: none"
	fi
	if [ -r "$EXTRA_LIST" ]; then
		echo "extra: $(strip_comments < "$EXTRA_LIST" | wc -l) entries in $EXTRA_LIST"
	else
		echo "extra: no $EXTRA_LIST"
	fi
	echo
	echo "hooks:"
	iptables -S INPUT 2>/dev/null | grep -- "-j $CHAIN" | sed 's/^/  INPUT: /' || true
	iptables -S DOCKER-USER 2>/dev/null | grep -- "-j $CHAIN" | sed 's/^/  DOCKER-USER: /' \
		|| echo "  DOCKER-USER: chain not present (no container port publishing?)"
	echo
	iptables -L "$CHAIN" -n -v 2>/dev/null || echo "no chain $CHAIN"
}

# Add or remove a single address by hand, to check the block from a machine
# you control. The entry is gone after the next update, which replaces the set;
# for a permanent entry use $EXTRA_LIST.
cmd_test_entry() {
	local op="$1" addr="${2:-}"

	[ -n "$addr" ] || die "usage: --test-$op <ip>"
	ipset list "$SET" -t >/dev/null 2>&1 || die "set $SET does not exist, run the script without options first"

	if [ "$op" = "add" ]; then
		ipset add "$SET" "$addr" -exist
		log "added $addr to $SET for testing - connections from it are now rejected on ports $PORTS"
		log "remove it again with: $(basename "$0") --test-del $addr"
	else
		ipset del "$SET" "$addr" -exist
		log "removed $addr from $SET"
	fi
}

cmd_remove() {
	remove_rules
	remove_ipv6_leftovers
	ipset destroy "$SET" 2>/dev/null || true
	log "rules and sets removed"
}

cmd_install_cron() {
	local self minute
	self="$(readlink -f "$0")"
	# Spread the daily download over the hour so not every host hits the
	# sources at the same second.
	minute=$(( RANDOM % 60 ))

	cat > /etc/cron.d/protonvpn-block <<EOF
# Refresh the ProtonVPN block list once a day and re-apply it after a reboot.
# Managed by protonvpn-block.sh --install-cron
SHELL=/bin/bash
PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
MAILTO=root

$minute 4 * * *  root  $self
@reboot          root  $self --from-cache
EOF
	chmod 0644 /etc/cron.d/protonvpn-block
	log "wrote /etc/cron.d/protonvpn-block (daily at 04:$(printf '%02d' $minute), plus @reboot)"
}

# --- main -------------------------------------------------------------------

main() {
	# Read-only commands: no root, no lock, no firewall tools. A dry run is
	# meant to be usable by hand before anything is applied.
	case "${1:-}" in
		--dry-run)
			need_tools curl
			trap cleanup EXIT
			cmd_update 1
			return 0 ;;
		--status)
			need_root; cmd_status; return 0 ;;
		-h|--help)
			sed -n '2,/^[^#]/p' "$0" | grep '^#'; return 0 ;;
	esac

	need_root
	need_tools curl ipset iptables flock
	trap cleanup EXIT

	# One instance at a time - a slow download must not collide with the next
	# cron run or with a manual invocation.
	exec 9>"$LOCK_FILE"
	flock -n 9 || die "another instance is already running"

	case "${1:-}" in
		"")             cmd_update 0 ;;
		--from-cache)   cmd_from_cache ;;
		--test-add)     cmd_test_entry add "${2:-}" ;;
		--test-del)     cmd_test_entry del "${2:-}" ;;
		--install-cron) cmd_install_cron ;;
		--remove)       cmd_remove ;;
		*)              die "unknown option: $1 (try --help)" ;;
	esac
}

main "$@"
