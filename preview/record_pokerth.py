#!/usr/bin/env python3
"""
PokerTH Widgets Headless-Demo (event-driven)
Flow: Startfenster -> Einstellungen -> Internet-Login -> Lobby (als Gast)

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


class WidgetsRecorder:
    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.script_dir = Path(__file__).resolve().parent
        self.output_dir = self.script_dir / "screenshots"
        self.video_file = self.script_dir / "pokerth_demo.mp4"
        self.ffmpeg_log = self.script_dir / "ffmpeg_pokerth.log"
        self.app_log = self.script_dir / "pokerth_client.log"

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
        self.settings_win_id: str | None = None
        self.login_win_id: str | None = None
        self.lobby_win_id: str | None = None

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

    async def _start_proc(self, *cmd: str, log_file: Path | None = None) -> asyncio.subprocess.Process:
        stdout = asyncio.subprocess.DEVNULL
        stderr = asyncio.subprocess.DEVNULL
        if log_file is not None:
            fh = log_file.open("wb")
            self._log_handles.append(fh)
            stdout = fh
            stderr = asyncio.subprocess.STDOUT
        return await asyncio.create_subprocess_exec(*cmd, stdout=stdout, stderr=stderr, env=self.env)

    async def _cleanup_strays(self) -> None:
        print("[0/6] Bereinige Reste vom letzten Lauf ...")
        for pattern in ("pokerth_client", f"Xvfb :{self.args.display_num}", "openbox"):
            await self._run_cmd("pkill", "-f", pattern, check=False)
        await asyncio.sleep(1.0)

    async def _start_services(self) -> None:
        print(f"[1/6] Starte Xvfb {self.display} ({self.display_res}x24) ...")
        self.xvfb_proc = await self._start_proc("Xvfb", self.display, "-screen", "0", f"{self.display_res}x24", "-ac")
        await asyncio.sleep(1.0)

        print("[2/6] Starte openbox ...")
        self.wm_proc = await self._start_proc("openbox")
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
        self.ffmpeg_proc = await self._start_proc(*ffmpeg_cmd, log_file=self.ffmpeg_log)
        await asyncio.sleep(1.0)

        print("[4/6] Starte PokerTH ...")
        cfg = Path.home() / ".pokerth" / "config.xml"
        if cfg.exists():
            await self._run_cmd(
                "sed",
                "-i",
                's|<InternetLoginMode value="[0-9]*"/>|<InternetLoginMode value="0"/>|',
                str(cfg),
                check=False,
            )

        self.app_proc = await self._start_proc(str(self.binary), log_file=self.app_log)

    async def _wait_for_main_window(self) -> None:
        print("[5/6] Warte auf PokerTH-Startfenster ...")
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
            raise RuntimeError("PokerTH-Fenster nicht gefunden")

        await self._run_cmd("xdotool", "windowactivate", "--sync", self.win_id, check=False)
        print(f"      Fenster-ID: {self.win_id} -> Warte 3s auf vollständiges Laden ...")
        await asyncio.sleep(3.0)

    async def _find_other_window(self, fallback: str | None) -> str | None:
        if not self.win_id:
            return fallback
        out = await self._run_cmd("xdotool", "search", "--onlyvisible", "--name", "PokerTH", check=False)
        candidates = [line.strip() for line in out.splitlines() if line.strip() and line.strip() != self.win_id]
        return candidates[0] if candidates else fallback

    async def _find_login_window(self, fallback: str | None) -> str | None:
        if not self.win_id:
            return fallback
        out = await self._run_cmd("xdotool", "search", "--onlyvisible", "--name", ".", check=False)
        candidates = [line.strip() for line in out.splitlines() if line.strip() and line.strip() != self.win_id]
        return candidates[0] if candidates else fallback

    async def _shot_window(self, name: str, win_id: str | None) -> None:
        if not win_id:
            raise RuntimeError(f"Kein Fenster für Screenshot {name}")
        path = self.output_dir / name
        await self._run_cmd("xdotool", "windowactivate", "--sync", win_id, check=False)
        await asyncio.sleep(0.5)
        await self._run_cmd("scrot", str(path))
        print(f"      Screenshot -> {path}")

    async def _key(self, win_id: str | None, *keys: str) -> None:
        if not win_id:
            raise RuntimeError("Kein aktives Fenster für Tastatureingabe")
        await self._run_cmd("xdotool", "windowactivate", "--sync", win_id, check=False)
        await asyncio.sleep(0.2)
        cmd = ["xdotool", "key", "--window", win_id]
        cmd.extend(keys)
        await self._run_cmd(*cmd)

    async def _capture_start(self) -> None:
        await self._shot_window("01_startfenster.png", self.win_id)

    async def _open_settings(self) -> None:
        print("      Öffne Einstellungen-Menü (Alt+E -> Enter) ...")
        await self._key(self.win_id, "alt+e")
        await asyncio.sleep(0.8)
        await self._run_cmd("xdotool", "key", "Return")
        await asyncio.sleep(2.0)
        self.settings_win_id = await self._find_other_window(self.win_id)

    async def _capture_settings(self) -> None:
        await self._shot_window("02_einstellungen.png", self.settings_win_id or self.win_id)

    async def _close_settings(self) -> None:
        await self._run_cmd("xdotool", "key", "Escape")
        await asyncio.sleep(1.0)

    async def _open_internet(self) -> None:
        print("      Öffne Internetspiel (Alt+2) ...")
        await self._key(self.win_id, "alt+2")
        await asyncio.sleep(3.0)
        self.login_win_id = await self._find_login_window(self.win_id)

    async def _capture_login(self) -> None:
        await self._shot_window("03_login.png", self.login_win_id or self.win_id)

    async def _guest_login(self) -> None:
        print("      Gast-Login (Tab Tab Tab Tab, Space, Tab, Return) ...")
        target = self.login_win_id or self.win_id
        await self._key(target, "Tab", "Tab", "Tab", "Tab")
        await asyncio.sleep(0.4)
        await self._run_cmd("xdotool", "key", "space")
        await asyncio.sleep(0.4)
        await self._run_cmd("xdotool", "key", "Tab")
        await asyncio.sleep(0.3)
        await self._run_cmd("xdotool", "key", "Return")

    async def _wait_for_lobby(self) -> None:
        print("      Warte auf Lobby-Verbindung (bis 35s) ...")
        if not self.win_id:
            return

        for i in range(1, 36):
            out = await self._run_cmd("xdotool", "search", "--onlyvisible", "--name", ".", check=False)
            for wid in [line.strip() for line in out.splitlines() if line.strip()]:
                if wid == self.win_id or wid == (self.login_win_id or ""):
                    continue
                title = await self._run_cmd("xdotool", "getwindowname", wid, check=False)
                if re.search(r"lobby|PokerTH|Spiel", title, flags=re.IGNORECASE):
                    self.lobby_win_id = wid
                    print(f"      Lobby-Fenster gefunden: {wid} ('{title.strip()}')")
                    return
            print(f"      ... warte {i}/35")
            await asyncio.sleep(1.0)

        out = await self._run_cmd("xdotool", "search", "--onlyvisible", "--name", ".", check=False)
        all_wins = [line.strip() for line in out.splitlines() if line.strip()]
        self.lobby_win_id = all_wins[0] if all_wins else self.win_id
        print(f"      Verwende aktuell aktives Fenster: {self.lobby_win_id}")

    async def _capture_lobby(self) -> None:
        await asyncio.sleep(2.0)
        await self._shot_window("04_lobby.png", self.lobby_win_id or self.win_id)

    def _build_events(self) -> list[TimerEvent]:
        now = time.monotonic()
        events: list[TimerEvent] = [
            TimerEvent(now + 0.0, "shot-start", self._capture_start),
            TimerEvent(now + 1.0, "open-settings", self._open_settings),
            TimerEvent(now + 3.5, "shot-settings", self._capture_settings),
            TimerEvent(now + 5.0, "close-settings", self._close_settings),
            TimerEvent(now + 6.5, "open-internet", self._open_internet),
            TimerEvent(now + 10.0, "shot-login", self._capture_login),
            TimerEvent(now + 11.5, "guest-login", self._guest_login),
            TimerEvent(now + 12.5, "wait-lobby", self._wait_for_lobby),
            TimerEvent(now + 16.0, "shot-lobby", self._capture_lobby),
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

    async def _stop_ffmpeg(self) -> None:
        if not self.ffmpeg_proc:
            return
        print("      Beende ffmpeg-Aufnahme ...")
        self.ffmpeg_proc.send_signal(signal.SIGINT)
        try:
            await asyncio.wait_for(self.ffmpeg_proc.wait(), timeout=10)
        except TimeoutError:
            self.ffmpeg_proc.kill()
            await self.ffmpeg_proc.wait()
        self.ffmpeg_proc = None

    async def _final_summary(self) -> None:
        print("\n╔══════════════════════════════════════╗")
        print("║         PokerTH Demo - Fertig        ║")
        print("╚══════════════════════════════════════╝")
        print("\nScreenshots:")
        shots = sorted(self.output_dir.glob("0*.png"))
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
            log_files={"ffmpeg": self.ffmpeg_log, "widgets": self.app_log},
            poll_interval=self.args.log_poll,
            batch_delay=self.args.log_delay,
        )

        try:
            await self._cleanup_strays()
            await self._start_services()
            self._log_task = asyncio.create_task(log_printer.run(self._log_stop))
            await self._wait_for_main_window()
            await self._run_event_loop()
            await asyncio.sleep(self.args.video_hold)
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
    parser = argparse.ArgumentParser(description="Event-driven PokerTH Widgets Preview Recorder")
    parser.add_argument("--display-num", type=int, default=99)
    parser.add_argument("--display-res", default="1024x768")
    parser.add_argument("--binary", default="/opt/pokerth_env/repos/pokerth-test/build/bin/pokerth_client")
    parser.add_argument("--log-poll", type=float, default=0.8, help="Sekunden zwischen Log-Polling")
    parser.add_argument("--log-delay", type=float, default=1.5, help="Sekunden verzögerte Log-Ausgabe")
    parser.add_argument("--video-hold", type=float, default=5.0, help="Zusätzliche Sekunden im Lobby-Video")
    return parser


async def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    recorder = WidgetsRecorder(args)
    return await recorder.run()


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))
