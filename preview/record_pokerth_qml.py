#!/usr/bin/env python3
"""
PokerTH QML-Client Headless-Demo (event-driven)
Flow: Startseite -> Internet-Login als Gast -> Lobby -> Lobby-Interaktionen

Benötigte apt-Pakete:
  sudo apt install xvfb openbox ffmpeg scrot xdotool
"""

from __future__ import annotations

import argparse
import asyncio
import contextlib
import heapq
import os
import re
import signal
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Awaitable, Callable


@dataclass(order=True)
class TimerEvent:
    run_at: float
    name: str = field(compare=False)
    action: Callable[[], Awaitable[None]] = field(compare=False)


class DeferredLogPrinter:
    def __init__(self, log_files: dict[str, Path], poll_interval: float, batch_delay: float) -> None:
        self.log_files = log_files
        self.poll_interval = poll_interval
        self.batch_delay = batch_delay
        self.offsets: dict[Path, int] = {path: 0 for path in log_files.values()}
        self.pending: list[tuple[float, str, str]] = []

    def _poll_once(self) -> None:
        now = time.monotonic()
        for source, path in self.log_files.items():
            if not path.exists():
                continue
            last = self.offsets[path]
            with path.open("rb") as fh:
                fh.seek(last)
                data = fh.read()
                self.offsets[path] = fh.tell()
            if not data:
                continue
            text = data.decode("utf-8", errors="replace")
            for line in text.splitlines():
                if line.strip():
                    self.pending.append((now, source, line))

    def _flush_ready(self, force: bool = False) -> None:
        now = time.monotonic()
        keep: list[tuple[float, str, str]] = []
        for ts, source, line in self.pending:
            if force or (now - ts) >= self.batch_delay:
                print(f"[log:{source}] {line}")
            else:
                keep.append((ts, source, line))
        self.pending = keep

    async def run(self, stop_event: asyncio.Event) -> None:
        while not stop_event.is_set():
            self._poll_once()
            self._flush_ready(force=False)
            await asyncio.sleep(self.poll_interval)
        self._poll_once()
        self._flush_ready(force=True)


class QmlRecorder:
    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.script_dir = Path(__file__).resolve().parent
        self.output_dir = self.script_dir / "screenshots_qml"
        self.video_file = self.script_dir / "pokerth_qml_demo.mp4"
        self.ffmpeg_log = self.script_dir / "ffmpeg_qml.log"
        self.app_log = self.script_dir / "pokerth_qml.log"

        self.display = f":{args.display_num}"
        self.display_res = args.display_res
        self.binary = Path(args.binary)

        self.env = os.environ.copy()
        self.env["DISPLAY"] = self.display

        self.xvfb_proc: asyncio.subprocess.Process | None = None
        self.wm_proc: asyncio.subprocess.Process | None = None
        self.ffmpeg_proc: asyncio.subprocess.Process | None = None
        self.app_proc: asyncio.subprocess.Process | None = None
        self._log_handles: list[object] = []

        self.win_id: str | None = None
        self.wx = 0
        self.wy = 0

        self._log_stop = asyncio.Event()
        self._log_task: asyncio.Task[None] | None = None

    async def _run_cmd(self, *cmd: str, check: bool = True) -> str:
        proc = await asyncio.create_subprocess_exec(
            *cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            env=self.env,
        )
        out_b, err_b = await proc.communicate()
        out = out_b.decode("utf-8", errors="replace")
        err = err_b.decode("utf-8", errors="replace")
        if check and proc.returncode != 0:
            raise RuntimeError(f"Befehl fehlgeschlagen ({proc.returncode}): {' '.join(cmd)}\n{err.strip()}")
        return out

    async def _start_proc(self, *cmd: str, log_file: Path | None = None) -> tuple[asyncio.subprocess.Process, object | None]:
        log_handle = None
        stdout = asyncio.subprocess.DEVNULL
        stderr = asyncio.subprocess.DEVNULL
        if log_file is not None:
            log_handle = log_file.open("wb")
            self._log_handles.append(log_handle)
            stdout = log_handle
            stderr = asyncio.subprocess.STDOUT

        proc = await asyncio.create_subprocess_exec(*cmd, stdout=stdout, stderr=stderr, env=self.env)
        return proc, log_handle

    async def _cleanup_strays(self) -> None:
        print("[0/6] Bereinige Reste vom letzten Lauf ...")
        for pattern in ("pokerth_qml-client", f"Xvfb :{self.args.display_num}", "openbox"):
            await self._run_cmd("pkill", "-f", pattern, check=False)
        await asyncio.sleep(1.0)

    async def _start_services(self) -> None:
        print(f"[1/6] Starte Xvfb {self.display} ({self.display_res}x24) ...")
        self.xvfb_proc, _ = await self._start_proc("Xvfb", self.display, "-screen", "0", f"{self.display_res}x24", "-ac")
        await asyncio.sleep(1.0)

        print("[2/6] Starte openbox ...")
        self.wm_proc, _ = await self._start_proc("openbox")
        await asyncio.sleep(1.0)

        print(f"[3/6] Starte ffmpeg-Aufnahme -> {self.video_file}")
        ffmpeg_cmd = (
            "ffmpeg",
            "-f", "x11grab",
            "-video_size", self.display_res,
            "-framerate", "15",
            "-i", self.display,
            "-c:v", "libx264", "-preset", "fast", "-crf", "23",
            "-pix_fmt", "yuv420p",
            "-profile:v", "baseline", "-level", "3.1",
            "-movflags", "+faststart",
            "-y", str(self.video_file),
        )
        self.ffmpeg_proc, _ = await self._start_proc(*ffmpeg_cmd, log_file=self.ffmpeg_log)
        await asyncio.sleep(1.0)

        print("[4/6] Starte QML-Client ...")
        cfg = Path.home() / ".pokerth" / "config.xml"
        if cfg.exists():
            await self._run_cmd(
                "sed",
                "-i",
                's|<InternetLoginMode value="[0-9]*"/>|<InternetLoginMode value="0"/>|',
                str(cfg),
                check=False,
            )

        self.app_proc, _ = await self._start_proc(str(self.binary), log_file=self.app_log)

    async def _wait_for_window(self) -> None:
        print("[5/6] Warte auf QML-Fenster ...")
        for i in range(1, 41):
            out = await self._run_cmd("xdotool", "search", "--onlyvisible", "--name", "PokerTH", check=False)
            win = next((line.strip() for line in out.splitlines() if line.strip()), "")
            if win:
                self.win_id = win
                break
            print(f"      ... {i}/40")
            await asyncio.sleep(1.0)

        if not self.win_id:
            debug_path = self.output_dir / "debug_no_window.png"
            await self._run_cmd("scrot", str(debug_path), check=False)
            raise RuntimeError("QML-Fenster nicht gefunden")

        geom = await self._run_cmd("xdotool", "getwindowgeometry", self.win_id)
        m = re.search(r"Position:\s*(\d+),(\d+)", geom)
        if not m:
            raise RuntimeError(f"Konnte Fensterposition nicht parsen: {geom!r}")
        self.wx, self.wy = int(m.group(1)), int(m.group(2))
        print(f"      Fenster {self.win_id}: Position={self.wx},{self.wy} -> Warte 8s (PreLoader) ...")
        await asyncio.sleep(8.0)

        await self._run_cmd("xdotool", "windowfocus", self.win_id, check=False)
        await asyncio.sleep(0.3)

    async def _shot(self, name: str) -> None:
        path = self.output_dir / name
        await asyncio.sleep(0.8)
        await self._run_cmd("scrot", "-p", str(path))
        print(f"      Screenshot -> {path}")
        if self.win_id:
            await self._run_cmd("xdotool", "windowfocus", self.win_id, check=False)
        await asyncio.sleep(0.1)

    async def _click(self, x: int, y: int, desc: str) -> None:
        print(f"      Klick ({x}, {y}) {desc}")
        await self._run_cmd("xdotool", "mousemove", "--sync", str(x), str(y))
        await asyncio.sleep(0.2)
        await self._run_cmd("xdotool", "click", "--clearmodifiers", "1")

    async def _stop_ffmpeg(self) -> None:
        if not self.ffmpeg_proc:
            return
        print("      Beende ffmpeg ...")
        self.ffmpeg_proc.send_signal(signal.SIGINT)
        try:
            await asyncio.wait_for(self.ffmpeg_proc.wait(), timeout=10)
        except TimeoutError:
            self.ffmpeg_proc.kill()
            await self.ffmpeg_proc.wait()
        self.ffmpeg_proc = None

    async def _final_summary(self) -> None:
        print("\n╔══════════════════════════════════════╗")
        print("║    PokerTH QML-Demo - Fertig         ║")
        print("╚══════════════════════════════════════╝")
        print("\nScreenshots:")
        shots = sorted(self.output_dir.glob("*.png"))
        if not shots:
            print("  (keine)")
        else:
            for p in shots:
                print(f"  {p.name}")
        print()
        if self.video_file.exists():
            print(f"Video: {self.video_file}")
        else:
            print(f"Video: nicht erstellt (Log: {self.ffmpeg_log})")

    def _build_events(self) -> list[TimerEvent]:
        wx, wy = self.wx, self.wy
        internet_x, internet_y = wx + 195, wy + 349
        guest_x, guest_y = wx + 195, wy + 507

        players_x, players_y = wx + 31, wy + 38
        close_players_x, close_players_y = wx + 365, wy + 32
        game_x, game_y = wx + 195, wy + 97
        back_x, back_y = wx + 31, wy + 38

        now = time.monotonic()
        events: list[TimerEvent] = [
            TimerEvent(now + 0.0, "shot-start", lambda: self._shot("01_startseite.png")),
            TimerEvent(now + 2.0, "click-internet", lambda: self._click(internet_x, internet_y, "(Internetspiel)")),
            TimerEvent(now + 4.0, "shot-login", lambda: self._shot("02_login.png")),
            TimerEvent(now + 5.5, "click-guest", lambda: self._click(guest_x, guest_y, "(Continue as Guest)")),
            TimerEvent(now + 8.5, "shot-lobby-start", lambda: self._shot("03_lobby_start.png")),
            TimerEvent(now + 9.8, "open-players", lambda: self._click(players_x, players_y, "(Spielerliste öffnen)")),
            TimerEvent(now + 11.5, "shot-players", lambda: self._shot("04_player_list.png")),
            TimerEvent(now + 12.8, "close-players", lambda: self._click(close_players_x, close_players_y, "(Spielerliste schließen)")),
            TimerEvent(now + 14.5, "shot-lobby-back", lambda: self._shot("05_lobby_back.png")),
            TimerEvent(now + 15.8, "select-game", lambda: self._click(game_x, game_y, "(Spiel selektieren)")),
            TimerEvent(now + 17.5, "shot-game-info", lambda: self._shot("06_game_info.png")),
            TimerEvent(now + 18.8, "close-game-info", lambda: self._click(back_x, back_y, "(Game-Info schließen)")),
            TimerEvent(now + 20.5, "shot-lobby-final", lambda: self._shot("07_lobby_final.png")),
        ]
        heapq.heapify(events)
        return events

    async def _run_event_loop(self) -> None:
        print("[6/6] Demo-Flow (Timer-Events) ...")
        events = self._build_events()
        while events:
            ev = heapq.heappop(events)
            delay = ev.run_at - time.monotonic()
            if delay > 0:
                await asyncio.sleep(delay)
            print(f"   -> Event: {ev.name}")
            await ev.action()

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

    async def run(self) -> int:
        self.output_dir.mkdir(parents=True, exist_ok=True)
        for png in self.output_dir.glob("*.png"):
            png.unlink(missing_ok=True)

        if self.video_file.exists():
            self.video_file.unlink(missing_ok=True)

        log_printer = DeferredLogPrinter(
            log_files={"ffmpeg": self.ffmpeg_log, "qml": self.app_log},
            poll_interval=self.args.log_poll,
            batch_delay=self.args.log_delay,
        )

        try:
            await self._cleanup_strays()
            await self._start_services()
            self._log_task = asyncio.create_task(log_printer.run(self._log_stop))
            await self._wait_for_window()
            await self._run_event_loop()
            await asyncio.sleep(self.args.lobby_hold)
            await self._stop_ffmpeg()
            await self._final_summary()
            return 0
        except Exception as exc:
            print(f"[FEHLER] {exc}", file=sys.stderr)
            return 1
        finally:
            self._log_stop.set()
            if self._log_task:
                with contextlib.suppress(Exception):
                    await self._log_task
            await self.cleanup()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Event-driven PokerTH QML Preview Recorder")
    parser.add_argument("--display-num", type=int, default=98)
    parser.add_argument("--display-res", default="600x1000")
    parser.add_argument("--binary", default="/opt/pokerth_env/repos/pokerth-test/build/bin/pokerth_qml-client")
    parser.add_argument("--log-poll", type=float, default=0.8, help="Sekunden zwischen Log-Polling")
    parser.add_argument("--log-delay", type=float, default=1.5, help="Sekunden verzögerte Log-Ausgabe")
    parser.add_argument("--lobby-hold", type=float, default=1.0, help="Zusätzliche Sekunden nach letztem Event")
    return parser


async def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    recorder = QmlRecorder(args)
    return await recorder.run()


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))
