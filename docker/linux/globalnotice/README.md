# pokerth_globalnotice

Command line tool for PokerTH server admins: it briefly logs in to a server with
an admin account, sends a **global notice** and disconnects again. The server
broadcasts the notice as a global chat message to every connected client (the
same function as `/gn <text>` in the lobby chat).

This file is the quick reference shipped inside the ZIP deploy; it is added to
the archive by
[`docker/linux/create_globalnotice_deploy.sh`](../create_globalnotice_deploy.sh).

## Archive contents

```
pokerth-globalnotice-linux-binary/
├── pokerth-globalnotice     # launcher (sets LD_LIBRARY_PATH/QT_PLUGIN_PATH)
├── bin/pokerth_globalnotice # the actual binary
├── bin/qt.conf              # Qt paths relative to the binary
├── lib/                     # bundled Qt/Boost/Protobuf/OpenSSL libraries
├── plugins/tls/             # Qt TLS backend (required for HTTPS)
├── COPYING                  # GNU AGPL v3
└── README.md                # this file
```

## Requirements

- Linux x86_64, glibc >= 2.39 (Ubuntu 24.04 / Linux Mint 22 or newer).
  The bundle does **not** contain glibc, so the glibc of the build container
  (Ubuntu 24.04 LTS) is the minimum requirement.
- A PokerTH account **with server admin rights**. Without the admin flag in the
  server database the server rejects the notice (exit code 3).

No root, no installation: unpack and run.

```bash
unzip pokerth-globalnotice-linux-x86_64-*.zip
cd pokerth-globalnotice-linux-binary
./pokerth-globalnotice --help
```

Always start the launcher `./pokerth-globalnotice`, not `bin/pokerth_globalnotice`
directly — otherwise the bundled libraries are not found.

## Usage

```bash
# Public server (address is taken from the pokerth.net server list)
./pokerth-globalnotice -u <admin> "Server restart in 5 minutes"

# Talk to a test server directly (no server list download)
./pokerth-globalnotice -u <admin> -H testserver.example.com --port 7234 --tls off \
    -m "Test notice"

# Show the available servers
./pokerth-globalnotice --list-servers
```

### Options

| Option | Meaning |
| --- | --- |
| `-u, --user <name>` | account name of the server admin (required) |
| `-p, --password <pw>` | password; prefer `$POKERTH_ADMIN_PASSWORD` or the interactive prompt |
| `-m, --message <text>` | notice text (may also be passed as the last argument without `-m`) |
| `-H, --host <host>` | address the server directly; skips the server list download |
| `--port <port>` | port (default 7236; only with `--host`) |
| `--tls on\|off` | TLS on/off (default `on`; only with `--host`) |
| `--serverlist <url>` | use a different server list (default `pokerth.net/serverlist.xml.z`) |
| `--server-id <id>` | pick an entry from the server list (default: first entry) |
| `--list-servers` | print the server list and exit |
| `--timeout <sec>` | network timeout (default 20) |
| `-y, --yes` | do not ask for confirmation before sending |

### Password

Order of precedence: `--password`, then the environment variable
`POKERTH_ADMIN_PASSWORD`, then an interactive prompt with echo turned off.
`--password` ends up in the shell history and in the process list, so for
scripts prefer:

```bash
POKERTH_ADMIN_PASSWORD='...' ./pokerth-globalnotice -y -u <admin> -m "..."
```

### Confirmation

The notice goes out to **all** connected players. On a terminal it is therefore
printed and has to be confirmed before it is sent; `-y` skips that, and in
non-interactive runs (cron, CI) the prompt is skipped automatically.

### Text length

The server broadcasts the notice as a chat message, so the chat limit of
128 bytes UTF-8 applies. Longer texts are truncated (with a warning on stderr).

## Exit codes

| Code | Meaning |
| --- | --- |
| 0 | notice was sent (or aborted at the prompt / `--help`) |
| 1 | usage error (missing option, empty text, unknown server) |
| 2 | network, TLS or login error (e.g. wrong password) |
| 3 | server rejected the notice — the account has no admin rights |

## Protocol flow (for troubleshooting)

The tool speaks the regular PokerTH protocol, just like the game client:

1. TCP connection, optionally TLS (server certificates are self-signed, only the
   transport is encrypted).
2. Wait for the server's `AnnounceMessage` (only `serverTypeInternetAuth` allows
   account logins).
3. `InitMessage` with `authenticatedLogin`, nickname and password (clear text
   inside the TLS tunnel, exactly like the GUI client does it).
4. `InitAckMessage` = logged in.
5. Send `AdminGlobalNoticeMessage`, wait for `AdminGlobalNoticeAckMessage`.
6. Disconnect.

The server logs every notice and every rejection (including account name and
database id) in its server log.

Common messages:

- `login failed: authentication failed (wrong user name or password)` — wrong
  account or password.
- `login blocked (rate limit / brute force protection)` — too many logins from
  this IP in a short time; wait a moment.
- `this server does not use authenticated logins` — the server runs without an
  account database (LAN/dedicated without auth), notices are not possible there.
- `The server rejected the global notice` — login succeeded, but the account is
  not a server admin.

## License

GNU Affero General Public License v3 (see `COPYING`), with the OpenSSL exception
as in the rest of the PokerTH source.
