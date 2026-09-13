# Screencast runner

Records a scripted PokerTH QML screencast: login → lobby → join a ranking game →
play hands while table theme, card deck and card back change on a timeline, with
emoji reactions thrown in. The recording is meant to be finished in post
production with slide effects, so the runner hands out a cue sheet with the exact
video position of every change.

## The split

| Where | What |
| --- | --- |
| `record_screencast.py` | Everything that is *supposed* to look like operation: Xvfb/openbox/ffmpeg, the login, the lobby, the join, the playing mode — all by **keyboard**. |
| `ScreencastDirector` (in the client) | Everything that must *not* look like operation: the style switches and the reactions. |

There are no click coordinates anywhere. The client is fully keyboard operable
(`docs/qml_client_keyboard_shortcuts.md`), and a key press does not move when a
layout changes:

| Step | Keys |
| --- | --- |
| Start page → login dialog | `Return` (the page focuses the "internet game" button) |
| Dialog → login form | `Return` (the dialog focuses "Login as User") |
| Submit the login | `Return` (user name and password come prefilled from the config) |
| Pick a game in the lobby | `↓` × `lobby.gameRowIndex`, then `Return` |
| Playing mode at the table | `F7` (Auto Check/Fold) |

The one place the keyboard alone does not reach is the lobby game list: the
lobby deliberately does not focus it on entering (that would select a row
unasked), and the number of Tab steps up to it is nothing a recording should
depend on. The director therefore asks for that focus explicitly
(`focusGameListRequested`) and reports when it has it.

The director lives in `src/gui/qt6-qml/cpp/screencastdirector.{h,cpp}`. It only
exists while the environment variable `POKERTH_SCREENCAST_CONFIG` points at a
script — the runner sets it. Without that variable nothing is constructed, no
context property is registered and no signal is connected.

That split is the point: switching a theme through the settings dialog would put
the settings dialog in the video. `StyleProvider::setTableStyle()` and friends
already write the config key, reload the assets and emit `changed()`, so the
running client rebinds on the spot without any dialog.

## Setup

```
sudo apt install xvfb openbox ffmpeg scrot xdotool
cp preview/screencast.example.json preview/screencast.json
$EDITOR preview/screencast.json      # login, timeline, duration
```

`screencast.json` is git-ignored because it holds the login credentials.

A built QML client is required (`recording.binary` in the script) and
`~/.pokerth/config.xml` has to exist — start the client once if it does not. The
runner backs that config up and restores it afterwards, because the director
writes the style keys into it while it runs.

```
python3 preview/record_screencast.py
```

## What a run produces

Everything lands next to the script in `preview/`:

| File | What |
| --- | --- |
| `pokerth_screencast.mp4` | The recording (x11grab → libx264, `recording.framerate`). |
| `pokerth_screencast_cues.json` | The cue sheet with video timecodes — the file for the post production. |
| `screencast_cues.jsonl` | The raw sheet the client appends to while it runs; named after the script. |
| `screenshots_screencast/` | A screenshot per step of the flow, numbered in order. |
| `pokerth_screencast.log` | The client log, including every `[SCREENCAST]` line. |
| `ffmpeg_screencast.log` | The ffmpeg output, if the video is missing. |

Video, logs, cue sheets and the screenshot directory are git-ignored. Each run
wipes its own leftovers first (old screenshots, the video, the raw cue sheet),
so a repeat never records into the last take.

Before anything is started, leftovers of an aborted run are cleared: stray
`pokerth_qml-client`, `Xvfb`, `openbox` and `ffmpeg` processes get SIGTERM,
then SIGKILL for whatever is still alive — a client hanging in its scene graph
teardown or an ffmpeg still muxing survives a term. A killed X server leaves its
lock behind, so `/tmp/.X<n>-lock` and `/tmp/.X11-unix/X<n>` are removed as well
once nothing holds the display any more; without that the new Xvfb refuses it
with *Server is already active*. The ffmpeg pattern is scoped to our own
display, so a recording running elsewhere is left alone.

## Options

The script carries the settings; the flags are only for a quick deviation
without editing it.

| Flag | Default |
| --- | --- |
| `--script PATH` | `preview/screencast.json` |
| `--binary PATH` | `recording.binary` from the script |
| `--display-num N` | `recording.displayNum` (98) |
| `--display-res WxH` | `recording.displayRes` (1920x1080) |
| `--config PATH` | `~/.pokerth/config.xml` |
| `--type-credentials` | off — types user name and password into the form instead of relying on the prefill |

## The script

One JSON file, two readers. The runner takes `login`, `lobby`, `game`, `recording` and `startStyle`; the
client takes `clock`, `timeline` and `cueSheet`. See `screencast.example.json` for the full set with comments.

The timeline entries are anchored either in time (`at`, seconds after the first
dealt hand) or on a hand (`atHand`). Fields that are left out stay untouched, so
one cue can swap the deck alone and the next one the whole look:

```json
{ "at": 130, "table": "pirates", "deck": "default4c", "back": "pirates" },
{ "at": 170, "reaction": "🔥" },
{ "at": 95,  "reaction": "😎", "visible": true }
```

`visible: true` opens the real reaction picker on camera, highlights the emoji
and only then sends it — that shows the viewer how the feature is used. Without
it the emoji goes out silently and only the effect above the seat is visible.

Style names are the directory names under `data/gfx/qml/table/`,
`data/gfx/qml/cards/` and `data/gfx/qml/backside/`.

## The client's own actions

Nothing is scripted here. Once the first hand is dealt the runner presses the
key for `game.playingMode`, and the client plays its own **Auto Check/Fold**
mode (`F7`): it checks when the check is really free and folds otherwise, never
raises, never calls.

The safety belt is inside that mode already
(`GameActionBar.runAutoAction` → `GameHandler::call(0)`): the call is refused if
the engine demands more than the amount that was seen, so an opponent raising in
between can never turn a check into an expensive call — it folds instead. That
is why the director does not carry an action policy of its own; a second
implementation would only drift away from the real one.

`autoCheckCall` (`F8`) calls any amount and keeps the seat alive longer, at the
price of losing chips on camera. `manual` (`F6`) leaves every turn to the server
timeout, `off` presses nothing at all.

## The cue sheet

The director appends every action to `<script>_cues.jsonl` with a wall clock
timestamp. The runner knows when ffmpeg started, measures the x11grab startup
latency against the real video length and writes
`pokerth_screencast_cues.json`:

```json
{ "videoTimecode": "00:02:14.320", "kind": "cue",
  "detail": "{\"table\":\"pirates\",\"deck\":\"default4c\",\"back\":\"pirates\"}" }
```

That is the list the slide effects are cut against. `kind` is `state` for the
flow markers (lobby, joinedGame, gameStarted), `cue` for the timeline steps,
and `marker` entries carry whatever label the script put on a cue. The client's
own check/fold actions are not in there — they are the client's own playing
mode, not scripted steps.

## When it goes wrong

The run aborts at the first step that does not confirm itself, and it always
leaves a screenshot in `screenshots_screencast/` plus the client log. The video
up to that point is kept and the cue sheet is still written, so an aborted take
is still readable.

| Screenshot | What did not happen | Where to look |
| --- | --- | --- |
| `debug_no_window.png` | No client window within 60 s | `recording.binary`, the client log |
| `03_login_failed.png` | No `state lobby` — the login did not go through | Credentials, server/port/TLS; try `--type-credentials` if `02b_login_form.png` shows an empty form |
| `03b_no_list_focus.png` | No `state gameListFocused` | The lobby came up in the compact layout — a wider `recording.windowWidth` |
| `04_join_failed.png` | No `state joinedGame` | Was there an open, non-full ranking game in the list at all? See `03c_game_selected.png` |
| `05_no_game_start.png` | No hand within `lobby.gameStartTimeout` | The table never filled up |

The client log is the running commentary: every `[SCREENCAST]` line is a state
marker, a cue or the script being loaded. A script the client rejects shows up
as `[SCREENCAST] script rejected:` with the reason, and the client then starts
as an ordinary client — so if a recording comes back with no style switches at
all, that line is the first thing to grep for.

## Caveats for a real ranking game

The recording joins a real ranking game on the real server, so the run is not
reproducible: how long the table takes to fill, who sits down and how the hands
go is out of the runner's hands. `lobby.gameStartTimeout` is the patience for the
first dealt hand; if it expires the run aborts with a screenshot rather than
recording an empty table. With pure check/fold the seat will eventually be
blinded away — keep `recording.durationAfterGameStart` shorter than that, or use
`game.playingMode: "autoCheckCall"`.
