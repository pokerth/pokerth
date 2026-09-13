#!/usr/bin/env python3
"""
PokerTH QML screencast recorder.

Flow: start page -> internet login as a user -> lobby (filtered to ranking
games) -> join the first open ranking game -> wait for the first hand. From
that moment on the in-app ScreencastDirector takes over: it switches table
theme, card deck and card back at the scripted times WITHOUT the settings
dialog ever appearing in the recording, and fires the emoji reactions.

Everything that is visible as an operation (login, lobby, join, the playing
mode) is driven here by KEYBOARD - the client is fully keyboard operable, see
docs/qml_client_keyboard_shortcuts.md, so the recording needs no click
coordinates that shift with every layout change. Everything that must not look
like an operation happens inside the client - see
src/gui/qt6-qml/cpp/screencastdirector.cpp.

The client's own turns are not scripted at all: F7 switches the client into its
own "Auto Check/Fold" playing mode, which checks when it is free and folds
otherwise.

The script file is shared: this runner reads login/lobby/game/recording and
startStyle, the client reads clock/timeline from the same JSON. See
screencast.example.json.

At the end a cue sheet is written next to the video which lists every switch as
a position IN THE VIDEO - that is the list the slide effects are cut against in
post production.

Required apt packages:
  sudo apt install xvfb openbox ffmpeg scrot xdotool
"""

from __future__ import annotations

import argparse
import asyncio
import base64
import contextlib
import json
import os
import re
import shutil
import signal
import sys
import time
from pathlib import Path
from typing import Any


# The playing mode keys at the game table (docs/qml_client_keyboard_shortcuts.md).
PLAYING_MODE_KEYS = {
    "manual": "F6",
    "autoCheckFold": "F7",
    "autoCheckCall": "F8",
}

# The lobby filter indices of LobbyHandler::setGameListFilterMode().
GAME_LIST_FILTERS = {
    "all": 0,
    "open": 1,
    "openNotFull": 2,
    "public": 3,
    "private": 4,
    "ranking": 5,
}


class LogTail:
    """Prints new lines of the client log while the recording runs."""

    def __init__(self, path: Path) -> None:
        self.path = path
        self.offset = 0

    def poll(self) -> list[str]:
        if not self.path.exists():
            return []
        with self.path.open("rb") as fh:
            fh.seek(self.offset)
            data = fh.read()
            self.offset = fh.tell()
        if not data:
            return []
        return [ln for ln in data.decode("utf-8", errors="replace").splitlines() if ln.strip()]


class ScreencastRecorder:
    def __init__(self, args: argparse.Namespace, script: dict[str, Any], script_path: Path) -> None:
        self.args = args
        self.script = script
        self.script_path = script_path

        self.script_dir = Path(__file__).resolve().parent
        self.output_dir = self.script_dir / "screenshots_screencast"
        self.video_file = self.script_dir / "pokerth_screencast.mp4"
        self.ffmpeg_log = self.script_dir / "ffmpeg_screencast.log"
        self.app_log = self.script_dir / "pokerth_screencast.log"

        rec = script.get("recording", {})
        self.binary = Path(args.binary or rec.get("binary", ""))
        self.display = f":{args.display_num if args.display_num is not None else rec.get('displayNum', 98)}"
        self.display_res = args.display_res or rec.get("displayRes", "1920x1080")
        self.framerate = int(rec.get("framerate", 25))
        self.win_w = int(rec.get("windowWidth", 1600))
        self.win_h = int(rec.get("windowHeight", 980))
        self.duration_after_start = float(rec.get("durationAfterGameStart", 420))
        self.final_hold = float(rec.get("finalHold", 3))

        m = re.match(r"^(\d+)x(\d+)$", self.display_res)
        if not m:
            raise ValueError(f"invalid displayRes: {self.display_res}")
        self.desktop_w, self.desktop_h = int(m.group(1)), int(m.group(2))

        self.login = script.get("login", {})
        self.lobby = script.get("lobby", {})

        # The cue sheet the client appends to. The same default the director
        # computes when the script does not name one.
        cue_sheet = script.get("cueSheet")
        self.cue_sheet_path = (
            Path(cue_sheet).expanduser()
            if cue_sheet
            else script_path.with_name(script_path.stem + "_cues.jsonl")
        )
        self.cue_report_path = self.script_dir / "pokerth_screencast_cues.json"

        self.env = os.environ.copy()
        self.env["DISPLAY"] = self.display
        self.env["POKERTH_SCREENCAST_CONFIG"] = str(script_path)
        # Stale .qmlc across builds makes new properties of old QML files read as
        # undefined - never worth debugging during a recording run.
        self.env["QML_DISABLE_DISK_CACHE"] = "1"

        self.config_path = Path(args.config).expanduser()
        self.config_backup: Path | None = None

        self.xvfb_proc: asyncio.subprocess.Process | None = None
        self.wm_proc: asyncio.subprocess.Process | None = None
        self.ffmpeg_proc: asyncio.subprocess.Process | None = None
        self.app_proc: asyncio.subprocess.Process | None = None
        self._log_handles: list[Any] = []

        self.win_id: str | None = None
        self.wx = self.wy = 0
        self.ww, self.wh = self.win_w, self.win_h

        self.ffmpeg_start_wall = 0.0
        self.ffmpeg_stop_wall = 0.0

    # ── process helpers ──────────────────────────────────────────────────────

    async def _run_cmd(self, *cmd: str, check: bool = True) -> str:
        proc = await asyncio.create_subprocess_exec(
            *cmd, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, env=self.env
        )
        out_b, err_b = await proc.communicate()
        if check and proc.returncode != 0:
            raise RuntimeError(
                f"command failed ({proc.returncode}): {' '.join(cmd)}\n"
                f"{err_b.decode('utf-8', errors='replace').strip()}"
            )
        return out_b.decode("utf-8", errors="replace")

    async def _start_proc(self, *cmd: str, log_file: Path | None = None) -> asyncio.subprocess.Process:
        stdout: Any = asyncio.subprocess.DEVNULL
        stderr: Any = asyncio.subprocess.DEVNULL
        if log_file is not None:
            fh = log_file.open("wb")
            self._log_handles.append(fh)
            stdout, stderr = fh, asyncio.subprocess.STDOUT
        return await asyncio.create_subprocess_exec(*cmd, stdout=stdout, stderr=stderr, env=self.env)

    async def _cleanup_strays(self) -> None:
        print("[0/8] cleaning up leftovers from the last run ...")
        for pattern in ("pokerth_qml-client", f"Xvfb {self.display}", "openbox"):
            await self._run_cmd("pkill", "-f", pattern, check=False)
        await asyncio.sleep(1.0)

    # ── client configuration ─────────────────────────────────────────────────

    async def _prepare_config(self) -> None:
        """Writes login, server and the opening look into config.xml.

        The director writes the style keys too while it runs, so the original
        config is backed up and restored afterwards - a screencast must not
        leave the developer's client on some random theme.
        """
        if not self.config_path.exists():
            raise RuntimeError(
                f"{self.config_path} not found - start the client once so that the config is created."
            )

        self.config_backup = self.config_path.with_suffix(".xml.screencast-backup")
        shutil.copy2(self.config_path, self.config_backup)
        print(f"[1/8] config backup -> {self.config_backup}")

        user = self.login.get("user", "")
        password = self.login.get("password", "")
        if not user or user == "CHANGEME":
            raise RuntimeError("login.user is not set in the script")
        encoded_password = base64.b64encode(password.encode("utf-8")).decode("ascii")

        tls = "1" if self.login.get("tls", False) else "0"
        server = self.login.get("server", "pthsrv.pokerth.net")
        port = int(self.login.get("port", 7234))

        filter_name = self.lobby.get("gameListFilter", "ranking")
        if filter_name not in GAME_LIST_FILTERS:
            raise RuntimeError(
                f"unknown lobby.gameListFilter {filter_name!r} "
                f"(one of {', '.join(sorted(GAME_LIST_FILTERS))})"
            )
        filter_index = GAME_LIST_FILTERS[filter_name]

        start_style = self.script.get("startStyle", {})

        replacements: list[tuple[str, str]] = [
            ("InternetLoginMode", "0"),
            # 1 = a manual server instead of the server list.
            ("InternetServerConfigMode", "1"),
            ("InternetServerAddress", server),
            ("InternetServerPort", str(port)),
            ("InternetServerUseTls", tls),
            ("MyName", user),
            ("InternetSavePassword", "1"),
            ("InternetLoginPassword", encoded_password),
            # The lobby then already opens filtered to ranking games, so no
            # filter fiddling is visible in the recording.
            ("DlgGameLobbyGameListFilterIndex", str(filter_index)),
            # The splash screen would eat the first seconds of the video.
            ("DisableSplashScreenOnStartup", "1"),
        ]
        for key, value in (
            ("QmlGameTableStyle", start_style.get("table", "")),
            ("QmlCardDeckStyle", start_style.get("deck", "")),
            ("QmlCardBackStyle", start_style.get("back", "")),
        ):
            if value:
                replacements.append((key, value))

        for key, value in replacements:
            escaped = value.replace("|", r"\|")
            await self._run_cmd(
                "sed", "-i", f's|<{key} value="[^"]*"/>|<{key} value="{escaped}"/>|', str(self.config_path),
                check=False,
            )
            grep_out = await self._run_cmd("grep", "-c", f"<{key} ", str(self.config_path), check=False)
            if grep_out.strip() in ("", "0"):
                print(f"      [WARN] key {key} missing in the config (old config revision?)")

        print(f"      login={user}  server={server}:{port} tls={tls}  lobby filter={filter_name}")

    async def _restore_config(self) -> None:
        if self.config_backup and self.config_backup.exists():
            shutil.copy2(self.config_backup, self.config_path)
            self.config_backup.unlink(missing_ok=True)
            print("      config restored.")

    # ── X / recording ────────────────────────────────────────────────────────

    async def _start_services(self) -> None:
        print(f"[2/8] starting Xvfb {self.display} ({self.display_res}x24) ...")
        self.xvfb_proc = await self._start_proc("Xvfb", self.display, "-screen", "0", f"{self.display_res}x24", "-ac")
        await asyncio.sleep(1.0)

        print("[3/8] starting openbox ...")
        self.wm_proc = await self._start_proc("openbox")
        await asyncio.sleep(1.0)

        print(f"[4/8] starting the ffmpeg recording -> {self.video_file}")
        ffmpeg_cmd = (
            "ffmpeg",
            "-f", "x11grab",
            "-video_size", self.display_res,
            "-framerate", str(self.framerate),
            "-i", self.display,
            "-c:v", "libx264", "-preset", "veryfast", "-crf", "20",
            "-pix_fmt", "yuv420p",
            "-movflags", "+faststart",
            "-y", str(self.video_file),
        )
        self.ffmpeg_proc = await self._start_proc(*ffmpeg_cmd, log_file=self.ffmpeg_log)
        self.ffmpeg_start_wall = time.time()
        await asyncio.sleep(1.5)

        print(f"[5/8] starting the QML client (POKERTH_SCREENCAST_CONFIG={self.script_path}) ...")
        self.app_proc = await self._start_proc(str(self.binary), log_file=self.app_log)

    async def _stop_ffmpeg(self) -> None:
        if not self.ffmpeg_proc:
            return
        print("      stopping ffmpeg ...")
        self.ffmpeg_proc.send_signal(signal.SIGINT)
        try:
            await asyncio.wait_for(self.ffmpeg_proc.wait(), timeout=15)
        except TimeoutError:
            self.ffmpeg_proc.kill()
            await self.ffmpeg_proc.wait()
        self.ffmpeg_stop_wall = time.time()
        self.ffmpeg_proc = None

    # ── window handling ──────────────────────────────────────────────────────

    async def _wait_for_window(self) -> None:
        print("[6/8] waiting for the QML window ...")
        for i in range(1, 61):
            out = await self._run_cmd("xdotool", "search", "--onlyvisible", "--name", "PokerTH", check=False)
            win = next((line.strip() for line in out.splitlines() if line.strip()), "")
            if win:
                self.win_id = win
                break
            if i % 5 == 0:
                print(f"      ... {i}/60")
            await asyncio.sleep(1.0)

        if not self.win_id:
            await self._shot("debug_no_window.png")
            raise RuntimeError("QML window not found")

        await self._apply_window_size()
        print(f"      window {self.win_id} at {self.wx},{self.wy} {self.ww}x{self.wh} - waiting 8s (PreLoader) ...")
        await asyncio.sleep(8.0)
        await self._run_cmd("xdotool", "windowfocus", self.win_id, check=False)
        await asyncio.sleep(0.3)

    async def _refresh_geometry(self) -> None:
        if not self.win_id:
            return
        out = await self._run_cmd("xdotool", "getwindowgeometry", "--shell", self.win_id, check=False)
        vals: dict[str, int] = {}
        for line in out.splitlines():
            if "=" in line:
                k, v = line.split("=", 1)
                if v.strip().isdigit():
                    vals[k.strip()] = int(v.strip())
        self.wx = vals.get("X", self.wx)
        self.wy = vals.get("Y", self.wy)
        self.ww = vals.get("WIDTH", self.ww)
        self.wh = vals.get("HEIGHT", self.wh)

    async def _apply_window_size(self) -> None:
        if not self.win_id:
            return
        px = max(0, (self.desktop_w - self.win_w) // 2)
        py = max(0, (self.desktop_h - self.win_h) // 2)
        await self._run_cmd("xdotool", "windowsize", "--sync", self.win_id, str(self.win_w), str(self.win_h), check=False)
        await self._run_cmd("xdotool", "windowmove", "--sync", self.win_id, str(px), str(py), check=False)
        await self._run_cmd("xdotool", "windowfocus", self.win_id, check=False)
        await asyncio.sleep(0.5)
        await self._refresh_geometry()

    # ── input ────────────────────────────────────────────────────────────────

    async def _shot(self, name: str) -> None:
        await self._run_cmd("scrot", "-p", str(self.output_dir / name), check=False)

    async def _focus(self) -> None:
        if self.win_id:
            await self._run_cmd("xdotool", "windowactivate", "--sync", self.win_id, check=False)
            await asyncio.sleep(0.05)

    async def _type(self, text: str) -> None:
        await self._focus()
        await self._run_cmd("xdotool", "type", "--clearmodifiers", "--delay", "12", text, check=False)

    async def _key(self, key: str, desc: str = "", repeat: int = 1, pause: float = 0.0) -> None:
        print(f"      key {key}{' x' + str(repeat) if repeat > 1 else ''} {desc}")
        await self._focus()
        for _ in range(repeat):
            await self._run_cmd("xdotool", "key", "--clearmodifiers", key, check=False)
            if pause:
                await asyncio.sleep(pause)

    # ── log synchronisation ──────────────────────────────────────────────────

    def _read_app_log(self) -> str:
        if not self.app_log.exists():
            return ""
        return self.app_log.read_text(encoding="utf-8", errors="replace")

    async def _wait_for_marker(self, markers: tuple[str, ...], timeout: float, what: str) -> bool:
        print(f"      waiting for {what} (max {timeout:.0f}s) ...")
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            text = self._read_app_log()
            if any(m in text for m in markers):
                return True
            await asyncio.sleep(0.5)
        return False

    # ── the flow ─────────────────────────────────────────────────────────────

    async def _do_login(self) -> None:
        """Start page -> login dialog -> lobby, by keyboard only.

        Three times Return is enough: the start page focuses the "internet game"
        button on activation, the dialog focuses "Login as User", and in the form
        the focus lands on the password field because the user name comes
        prefilled out of the config we just wrote. The Return in the form is the
        default button of the login (ServerConnectionDialog.qml).
        """
        print("[7/8] login (keyboard) ...")
        await self._shot("01_start_page.png")
        await self._key("Return", "(internet game)")
        await asyncio.sleep(2.0)
        await self._shot("02_login_dialog.png")

        await self._key("Return", "(login as user)")
        await asyncio.sleep(1.2)
        await self._shot("02b_login_form.png")

        # The safety net for a config whose credential keys did not take: type
        # them in explicitly instead of submitting an empty form.
        if self.args.type_credentials:
            await self._key("shift+Tab", "(into the user name field)")
            await self._key("ctrl+a")
            await self._type(self.login.get("user", ""))
            await self._key("Tab", "(into the password field)")
            await self._key("ctrl+a")
            await self._type(self.login.get("password", ""))
            await asyncio.sleep(0.3)

        await self._key("Return", "(submit login)")

        # The director reports the state transitions; the [NAV] lines of the QML
        # pages are commented out and are no reliable signal.
        if not await self._wait_for_marker(
            ("[SCREENCAST] state lobby",), 45.0, "the lobby"
        ):
            await self._shot("03_login_failed.png")
            raise RuntimeError(
                "login not detected - the director never reported 'state lobby'. "
                "If the login form stayed empty, try --type-credentials."
            )
        await self._shot("03_lobby.png")

    async def _join_ranking_game(self) -> None:
        """Picks a game in the list with the arrow keys and joins with Return.

        The lobby deliberately does not focus its game list by itself, so the
        director puts the focus there (focusGameListRequested) and says when it
        did - that beats guessing how many Tab steps lead to the list.
        """
        if not await self._wait_for_marker(
            ("[SCREENCAST] state gameListFocused",), 20.0, "the focus in the game list"
        ):
            await self._shot("03b_no_list_focus.png")
            raise RuntimeError("the director never reported 'state gameListFocused'")

        row = int(self.lobby.get("gameRowIndex", 0))
        if row > 0:
            await self._key("Down", "(select a game)", repeat=row, pause=0.45)
            await asyncio.sleep(0.5)
        await self._shot("03c_game_selected.png")

        print("      joining (the lobby filter is already on ranking games) ...")
        await self._key("Return", "(join game)")
        await asyncio.sleep(0.8)

        # A private game opens the password popup, which focuses its input field
        # and joins on Return.
        game_password = self.lobby.get("gamePassword", "")
        if game_password:
            await self._type(game_password)
            await self._key("Return", "(game password)")
            await asyncio.sleep(0.6)

        if not await self._wait_for_marker(
            ("[SCREENCAST] state joinedGame",),
            float(self.lobby.get("joinTimeout", 30)),
            "the game join",
        ):
            await self._shot("04_join_failed.png")
            raise RuntimeError(
                "join not detected - the director never reported 'state joinedGame'. "
                "Is there an open, non-full ranking game in the list at all?"
            )
        await self._shot("04_game_wait.png")

    async def _wait_for_first_hand(self) -> None:
        # The director starts its clock with the first dealt hand and says so in
        # the log - that is the one marker that really matches the script.
        timeout = float(self.lobby.get("gameStartTimeout", 900))
        if not await self._wait_for_marker(
            ("[SCREENCAST] game clock started with hand",),
            timeout,
            "the first dealt hand",
        ):
            await self._shot("05_no_game_start.png")
            raise RuntimeError(
                f"no hand was dealt within {timeout:.0f}s - "
                "the table probably never filled up (a real ranking game needs real opponents)"
            )
        await self._shot("05_first_hand.png")

    async def _apply_playing_mode(self) -> None:
        """Hands the client's own turns over to its own Auto Check/Fold mode.

        That mode checks when the check is really free and folds otherwise
        (GameActionBar.runAutoAction) - exactly the boring policy a screencast
        wants, and nothing that had to be scripted a second time.
        """
        mode = self.script.get("game", {}).get("playingMode", "autoCheckFold")
        if mode == "off":
            print("      playing mode: left as it is (the script says off)")
            return
        if mode not in PLAYING_MODE_KEYS:
            raise RuntimeError(
                f"unknown game.playingMode {mode!r} "
                f"(one of {', '.join(sorted(PLAYING_MODE_KEYS))}, or off)"
            )
        await self._key(PLAYING_MODE_KEYS[mode], f"(playing mode {mode})")
        await asyncio.sleep(0.6)
        await self._shot("05b_playing_mode.png")

    async def _run_timeline(self) -> None:
        print(f"[8/8] the director is playing the timeline ({self.duration_after_start:.0f}s) ...")
        tail = LogTail(self.app_log)
        deadline = time.monotonic() + self.duration_after_start
        finished = False
        while time.monotonic() < deadline:
            for line in tail.poll():
                if "[SCREENCAST]" in line:
                    print(f"      {line.strip()}")
                    if "timeline finished" in line:
                        finished = True
            if finished:
                print("      the timeline is through before the planned duration.")
                break
            if self.app_proc and self.app_proc.returncode is not None:
                raise RuntimeError("the client exited during the recording")
            await asyncio.sleep(0.5)
        await self._shot("06_end.png")
        await asyncio.sleep(self.final_hold)

    # ── the cue sheet for the post production ────────────────────────────────

    async def _video_duration(self) -> float | None:
        if not shutil.which("ffprobe") or not self.video_file.exists():
            return None
        out = await self._run_cmd(
            "ffprobe", "-v", "error", "-show_entries", "format=duration",
            "-of", "default=noprint_wrappers=1:nokey=1", str(self.video_file),
            check=False,
        )
        try:
            return float(out.strip())
        except ValueError:
            return None

    async def _write_cue_report(self) -> None:
        """Turns the director's wall clock cues into positions in the video."""
        if not self.cue_sheet_path.exists():
            print(f"      [WARN] no cue sheet at {self.cue_sheet_path}")
            return

        entries: list[dict[str, Any]] = []
        for line in self.cue_sheet_path.read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if not line:
                continue
            with contextlib.suppress(json.JSONDecodeError):
                entries.append(json.loads(line))

        # x11grab needs a moment before the first frame lands. The difference
        # between the wall clock span and the real video length is exactly that
        # startup latency - subtracting it makes the positions match the frames.
        duration = await self._video_duration()
        wall_span = max(0.0, self.ffmpeg_stop_wall - self.ffmpeg_start_wall)
        latency = 0.0
        if duration is not None and wall_span > duration:
            latency = wall_span - duration

        cues = []
        for entry in entries:
            wall_ms = entry.get("wallMs")
            if wall_ms is None:
                continue
            video_t = (wall_ms / 1000.0) - self.ffmpeg_start_wall - latency
            cues.append({
                "videoSeconds": round(video_t, 3),
                "videoTimecode": self._timecode(video_t),
                "scriptSeconds": entry.get("t"),
                "hand": entry.get("hand"),
                "kind": entry.get("kind"),
                "detail": entry.get("detail"),
            })

        report = {
            "video": str(self.video_file),
            "videoDurationSeconds": duration,
            "ffmpegStartupLatencySeconds": round(latency, 3),
            "script": str(self.script_path),
            "cues": cues,
        }
        self.cue_report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")

        print(f"\nCue sheet -> {self.cue_report_path}")
        for cue in cues:
            print(f"  {cue['videoTimecode']}  {cue['kind']:<7}  {cue['detail']}")

    @staticmethod
    def _timecode(seconds: float) -> str:
        if seconds < 0:
            seconds = 0.0
        minutes, sec = divmod(seconds, 60)
        hours, minutes = divmod(int(minutes), 60)
        return f"{hours:02d}:{minutes:02d}:{sec:06.3f}"

    # ── lifecycle ────────────────────────────────────────────────────────────

    async def cleanup(self) -> None:
        await self._stop_ffmpeg()
        for proc in (self.app_proc, self.wm_proc, self.xvfb_proc):
            if proc and proc.returncode is None:
                proc.terminate()
        for proc in (self.app_proc, self.wm_proc, self.xvfb_proc):
            if proc:
                try:
                    await asyncio.wait_for(proc.wait(), timeout=5)
                except TimeoutError:
                    proc.kill()
                    await proc.wait()
        for handle in self._log_handles:
            with contextlib.suppress(Exception):
                handle.close()
        self._log_handles.clear()
        await self._restore_config()

    async def run(self) -> int:
        self.output_dir.mkdir(parents=True, exist_ok=True)
        for png in self.output_dir.glob("*.png"):
            png.unlink(missing_ok=True)
        self.video_file.unlink(missing_ok=True)
        # The director appends, so an old sheet would be carried over.
        self.cue_sheet_path.unlink(missing_ok=True)

        if not self.binary.exists():
            print(f"[ERROR] client binary not found: {self.binary}", file=sys.stderr)
            return 1

        try:
            await self._cleanup_strays()
            await self._prepare_config()
            await self._start_services()
            await self._wait_for_window()
            await self._do_login()
            await self._join_ranking_game()
            await self._wait_for_first_hand()
            await self._apply_playing_mode()
            await self._run_timeline()
            await self._stop_ffmpeg()
            await self._write_cue_report()
            print(f"\nVideo: {self.video_file}")
            return 0
        except Exception as exc:
            print(f"[ERROR] {exc}", file=sys.stderr)
            with contextlib.suppress(Exception):
                await self._stop_ffmpeg()
                await self._write_cue_report()
            return 1
        finally:
            await self.cleanup()


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="PokerTH QML screencast recorder")
    p.add_argument("--script", default=str(Path(__file__).resolve().parent / "screencast.json"),
                   help="the JSON script (shared with the in-app director)")
    p.add_argument("--binary", default=None, help="overrides recording.binary")
    p.add_argument("--display-num", type=int, default=None, help="overrides recording.displayNum")
    p.add_argument("--display-res", default=None, help="overrides recording.displayRes")
    p.add_argument("--config", default=str(Path("~/.pokerth/config.xml").expanduser()))
    p.add_argument("--type-credentials", action="store_true",
                   help="type user name and password into the form instead of "
                        "relying on the values prefilled from the config")
    return p


async def main() -> int:
    args = build_parser().parse_args()
    script_path = Path(args.script).expanduser().resolve()
    if not script_path.exists():
        print(
            f"[ERROR] script not found: {script_path}\n"
            f"        copy preview/screencast.example.json to {script_path.name} "
            f"and put your login in it.",
            file=sys.stderr,
        )
        return 1
    script = json.loads(script_path.read_text(encoding="utf-8"))
    return await ScreencastRecorder(args, script, script_path).run()


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))
