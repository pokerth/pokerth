#!/usr/bin/env python3
"""
PokerTH QML-Client - Lokales Spiel Preview (event-driven)
Flow: Startseite -> Lokales Spiel starten -> 2 Haende spielen -> in Hand 3 anlaufen -> zurueck

Benoetigte apt-Pakete:
  sudo apt install xvfb openbox ffmpeg scrot xdotool pulseaudio pulseaudio-utils
"""

from __future__ import annotations

import argparse
import asyncio
import contextlib
import heapq
import os
import re
import shutil
import signal
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Awaitable, Callable


TIMING_PROFILES: dict[str, dict[str, float]] = {
    "fast": {
        "mode_toggle_interval": 7.0,
        "fullscreen_rest_sec": 3.5,
        "hand_switch_delay": 5.0,
        "hand3_exit_delay": 3.0,
    },
    "normal": {
        "mode_toggle_interval": 10.0,
        "fullscreen_rest_sec": 5.5,
        "hand_switch_delay": 7.0,
        "hand3_exit_delay": 4.0,
    },
    "slow": {
        "mode_toggle_interval": 13.0,
        "fullscreen_rest_sec": 7.0,
        "hand_switch_delay": 9.0,
        "hand3_exit_delay": 5.0,
    },
}


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


class LocalGameRecorder:
    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.script_dir = Path(__file__).resolve().parent
        self.output_dir = self.script_dir / "screenshots_localgame"
        self.video_file = self.script_dir / "pokerth_qml_localgame_demo.mp4"
        self.ffmpeg_log = self.script_dir / "ffmpeg_localgame.log"
        self.app_log = self.script_dir / "pokerth_localgame.log"
        self.pulse_log = self.script_dir / "pulseaudio_localgame.log"
        self.state_file = self.script_dir / ".preview_view_mode"

        self.display_num = args.display_num
        self.display = f":{args.display_num}"
        self.desktop_w = args.desktop_w
        self.desktop_h = args.desktop_h
        self.display_res = f"{args.desktop_w}x{args.desktop_h}"
        self.portrait_w = args.portrait_w
        self.portrait_h = args.portrait_h

        self.binary = Path(args.binary)

        self.env = os.environ.copy()
        self.env["DISPLAY"] = self.display
        self.env["PULSE_LATENCY_MSEC"] = "60"

        self.xvfb_proc: asyncio.subprocess.Process | None = None
        self.wm_proc: asyncio.subprocess.Process | None = None
        self.ffmpeg_proc: asyncio.subprocess.Process | None = None
        self.app_proc: asyncio.subprocess.Process | None = None
        self._log_handles: list[object] = []

        self.win_id: str | None = None
        self.current_view_mode = "portrait"

        self.wx = 0
        self.wy = 0
        self.ww = 0
        self.wh = 0

        self.audio_enabled = False
        self.audio_source = ""
        self.audio_runtime_dir = ""

        self._log_stop = asyncio.Event()
        self._log_task: asyncio.Task[None] | None = None
        self._log_overlay_stop = asyncio.Event()
        self._log_overlay_task: asyncio.Task[None] | None = None
        self._log_overlay_visible = False

    async def _run_cmd(self, *cmd: str, env: dict[str, str] | None = None, check: bool = True) -> str:
        proc = await asyncio.create_subprocess_exec(
            *cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            env=env or self.env,
        )
        out_b, err_b = await proc.communicate()
        out = out_b.decode("utf-8", errors="replace")
        err = err_b.decode("utf-8", errors="replace")
        if check and proc.returncode != 0:
            raise RuntimeError(f"Befehl fehlgeschlagen ({proc.returncode}): {' '.join(cmd)}\n{err.strip()}")
        return out

    async def _start_proc(self, *cmd: str, log_file: Path | None = None, env: dict[str, str] | None = None) -> asyncio.subprocess.Process:
        stdout = asyncio.subprocess.DEVNULL
        stderr = asyncio.subprocess.DEVNULL
        if log_file is not None:
            fh = log_file.open("wb")
            self._log_handles.append(fh)
            stdout = fh
            stderr = asyncio.subprocess.STDOUT
        return await asyncio.create_subprocess_exec(*cmd, stdout=stdout, stderr=stderr, env=env or self.env)

    async def _cleanup_strays(self) -> None:
        print("[0/6] Bereinige Reste vom letzten Lauf ...")
        for pattern in ("pokerth_qml-client", f"Xvfb :{self.display_num}", "openbox"):
            await self._run_cmd("pkill", "-f", pattern, check=False)
        await asyncio.sleep(1.0)

    async def _setup_virtual_audio(self) -> None:
        print("[3/6] Initialisiere Audio ...")
        if shutil.which("pulseaudio") is None or shutil.which("pactl") is None:
            print("      Hinweis: pulseaudio/pactl fehlt - Aufnahme laeuft ohne Audio.")
            return

        self.audio_runtime_dir = tempfile.mkdtemp(prefix="pulse-runtime.", dir=str(self.script_dir))
        pulse_runtime = str(Path(self.audio_runtime_dir) / "pulse")
        os.makedirs(pulse_runtime, exist_ok=True)

        audio_env = self.env.copy()
        audio_env["XDG_RUNTIME_DIR"] = self.audio_runtime_dir
        audio_env["PULSE_RUNTIME_PATH"] = pulse_runtime
        audio_env["PULSE_SERVER"] = f"unix:{pulse_runtime}/native"

        await self._run_cmd(
            "pulseaudio",
            "--daemonize=yes",
            "--exit-idle-time=-1",
            "--disable-shm=true",
            f"--log-target=file:{self.pulse_log}",
            env=audio_env,
            check=False,
        )

        for _ in range(20):
            info = await self._run_cmd("pactl", "info", env=audio_env, check=False)
            if "Server Name" in info:
                break
            await asyncio.sleep(0.2)
        else:
            print("      Hinweis: PulseAudio antwortet nicht - ohne Audio.")
            await self._run_cmd("pulseaudio", "--kill", env=audio_env, check=False)
            return

        await self._run_cmd(
            "pactl",
            "load-module",
            "module-null-sink",
            "sink_name=pokerth_preview",
            "sink_properties=device.description=PokerTHPreview",
            env=audio_env,
            check=False,
        )
        await self._run_cmd("pactl", "set-default-sink", "pokerth_preview", env=audio_env, check=False)

        self.audio_enabled = True
        self.audio_source = "pokerth_preview.monitor"

        self.env.update(
            {
                "XDG_RUNTIME_DIR": self.audio_runtime_dir,
                "PULSE_RUNTIME_PATH": pulse_runtime,
                "PULSE_SERVER": f"unix:{pulse_runtime}/native",
            }
        )

        print(f"      Audio aktiv: {self.audio_source}")

    async def _start_services(self) -> None:
        print(f"[1/6] Starte Xvfb {self.display} ({self.display_res}x24) ...")
        self.xvfb_proc = await self._start_proc("Xvfb", self.display, "-screen", "0", f"{self.display_res}x24", "-ac")
        await asyncio.sleep(1.0)

        print("[2/6] Starte openbox ...")
        self.wm_proc = await self._start_proc("openbox")
        await asyncio.sleep(1.0)

        await self._setup_virtual_audio()

        print(f"[4/6] Starte ffmpeg-Aufnahme -> {self.video_file}")
        ffmpeg_args = [
            "ffmpeg",
            "-f",
            "x11grab",
            "-video_size",
            self.display_res,
            "-framerate",
            "15",
            "-i",
            self.display,
        ]

        if self.audio_enabled:
            ffmpeg_args += [
                "-thread_queue_size",
                "512",
                "-sample_rate",
                "44100",
                "-channels",
                "2",
                "-fragment_size",
                "8820",
                "-f",
                "pulse",
                "-i",
                self.audio_source,
            ]

        ffmpeg_args += [
            "-c:v",
            "libx264",
            "-preset",
            "fast",
            "-crf",
            "23",
            "-pix_fmt",
            "yuv420p",
            "-profile:v",
            "baseline",
            "-level",
            "3.1",
        ]

        if self.audio_enabled:
            adelay = str(self.args.audio_sync_delay_ms)
            ffmpeg_args += [
                "-c:a",
                "aac",
                "-b:a",
                "128k",
                "-ar",
                "44100",
                "-af",
                f"adelay={adelay}|{adelay}",
            ]

        ffmpeg_args += ["-movflags", "+faststart", "-y", str(self.video_file)]

        self.ffmpeg_proc = await self._start_proc(*ffmpeg_args, log_file=self.ffmpeg_log)
        await asyncio.sleep(1.0)

        print("[5/6] Starte QML-Client ...")
        self.app_proc = await self._start_proc(str(self.binary), log_file=self.app_log)

    async def _wait_for_window(self) -> None:
        print("      Warte auf QML-Fenster ...")
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
        if m:
            self.wx, self.wy = int(m.group(1)), int(m.group(2))

        print(f"      Fenster {self.win_id}: Position={self.wx},{self.wy} - Warte 8s (PreLoader) ...")
        await asyncio.sleep(8.0)
        await self._run_cmd("xdotool", "windowfocus", self.win_id, check=False)
        await asyncio.sleep(0.3)

    async def _refresh_window_geometry(self) -> None:
        if not self.win_id:
            return
        out = await self._run_cmd("xdotool", "getwindowgeometry", "--shell", self.win_id, check=False)
        vals: dict[str, int] = {}
        for line in out.splitlines():
            if "=" not in line:
                continue
            k, v = line.split("=", 1)
            if v.strip().isdigit():
                vals[k.strip()] = int(v.strip())
        self.wx = vals.get("X", self.wx)
        self.wy = vals.get("Y", self.wy)
        self.ww = vals.get("WIDTH", self.ww)
        self.wh = vals.get("HEIGHT", self.wh)

    async def _update_click_coords(self) -> dict[str, int]:
        await self._refresh_window_geometry()

        if self.ww <= 0 or self.wh <= 0:
            self.ww, self.wh = self.portrait_w, self.portrait_h

        coords = {
            "lokalgame_x": self.wx + (self.ww * 195 // 390),
            "lokalgame_y": self.wy + (self.wh * 405 // 844),
            "spielstart_x": self.wx + (self.ww * 288 // 390),
            "spielstart_y": self.wy + (self.wh * 671 // 844),
            "door_x": self.wx + (self.ww * 19 // 390),
            "door_y": self.wy + (self.wh * 19 // 844),
            "fold_x": self.wx + (self.ww * 68 // 390),
            "call_x": self.wx + (self.ww * 195 // 390),
            "raise_x": self.wx + (self.ww * 322 // 390),
            "action_y": self.wy + (self.wh * 789 // 844),
            "half_pot_x": self.wx + (self.ww * 151 // 390),
            "half_pot_y": self.wy + (self.wh * 748 // 844),
            "raise_active_y": self.wy + (self.wh * 792 // 844),
        }

        if (
            self.current_view_mode == "fullscreen"
            and self.desktop_w == 1440
            and self.desktop_h == 900
        ):
            coords["fold_x"] = 721 - 163
            coords["call_x"] = 721
            coords["raise_x"] = 1189
            coords["action_y"] = 861
            coords["half_pot_x"] = 558
            coords["half_pot_y"] = 817
            coords["raise_active_y"] = 864

        return coords

    async def _click(self, x: int, y: int, desc: str, fast: bool = False) -> None:
        print(f"      Klick ({x}, {y}) {desc}")
        await self._run_cmd("xdotool", "mousemove", "--sync", str(x), str(y))
        if not fast:
            await asyncio.sleep(0.2)
        await self._run_cmd("xdotool", "click", "--clearmodifiers", "1")

    async def _shot(self, name: str) -> None:
        path = self.output_dir / name
        await asyncio.sleep(0.8)
        await self._run_cmd("scrot", "-p", str(path))
        print(f"      Screenshot -> {path}")
        if self.win_id:
            await self._run_cmd("xdotool", "windowfocus", self.win_id, check=False)
        await asyncio.sleep(0.1)

    async def _wait(self, seconds: float) -> None:
        print(f"      Warte {seconds}s ...")
        await asyncio.sleep(seconds)

    async def _set_auto_check_fold_mode(self) -> None:
        if self.win_id:
            print("      Aktiviere Auto Check/Fold ...")
            await self._run_cmd("xdotool", "key", "--window", self.win_id, "alt+f", check=False)

    async def _toggle_logs_shortcut(self) -> None:
        if not self.win_id:
            return
        await self._run_cmd("xdotool", "key", "--window", self.win_id, "alt+l", check=False)
        self._log_overlay_visible = not self._log_overlay_visible

    async def _run_periodic_log_overlay(self) -> None:
        if not self.win_id:
            return

        print(
            "      Log-Overlay zyklisch: "
            f"alle {self.args.log_cycle_interval}s fuer {self.args.log_cycle_visible}s"
        )

        while not self._log_overlay_stop.is_set():
            # Erst nach einem kompletten Intervall einblenden.
            try:
                await asyncio.wait_for(self._log_overlay_stop.wait(), timeout=self.args.log_cycle_interval)
                break
            except TimeoutError:
                pass

            await self._toggle_logs_shortcut()
            try:
                await asyncio.wait_for(self._log_overlay_stop.wait(), timeout=self.args.log_cycle_visible)
            except TimeoutError:
                pass

            if self._log_overlay_visible:
                await self._toggle_logs_shortcut()

    async def _start_periodic_log_overlay(self) -> None:
        if self._log_overlay_task is not None:
            return
        self._log_overlay_stop.clear()
        self._log_overlay_task = asyncio.create_task(self._run_periodic_log_overlay())

    async def _stop_periodic_log_overlay(self) -> None:
        self._log_overlay_stop.set()
        if self._log_overlay_task:
            with contextlib.suppress(Exception):
                await self._log_overlay_task
            self._log_overlay_task = None
        if self._log_overlay_visible:
            await self._toggle_logs_shortcut()

    async def _press_fullscreen_toggle(self) -> None:
        if not self.win_id:
            return
        await self._run_cmd("xdotool", "windowactivate", "--sync", self.win_id, check=False)
        await self._run_cmd("xdotool", "key", "--clearmodifiers", "F11", check=False)
        await asyncio.sleep(0.2)

    async def _apply_portrait_mode(self) -> None:
        if not self.win_id:
            return
        if self.current_view_mode == "fullscreen":
            await self._press_fullscreen_toggle()

        px = (self.desktop_w - self.portrait_w) // 2
        py = (self.desktop_h - self.portrait_h) // 2
        await self._run_cmd("xdotool", "windowsize", "--sync", self.win_id, str(self.portrait_w), str(self.portrait_h), check=False)
        await self._run_cmd("xdotool", "windowmove", "--sync", self.win_id, str(px), str(py), check=False)
        await self._run_cmd("xdotool", "windowfocus", self.win_id, check=False)
        self.current_view_mode = "portrait"
        self.state_file.write_text("portrait\n", encoding="utf-8")

    async def _apply_fullscreen_mode(self) -> None:
        if not self.win_id:
            return
        await self._press_fullscreen_toggle()
        await self._run_cmd("xdotool", "windowfocus", self.win_id, check=False)
        self.current_view_mode = "fullscreen"
        self.state_file.write_text("fullscreen\n", encoding="utf-8")

    async def _toggle_view_mode(self) -> None:
        if self.current_view_mode == "portrait":
            print("      ViewMode-Wechsel: portrait -> fullscreen")
            await self._apply_fullscreen_mode()
        else:
            print("      ViewMode-Wechsel: fullscreen -> portrait")
            await self._apply_portrait_mode()

    async def _run_round_mode_cycle(self, label: str) -> None:
        print(f"      {label}: warte {self.args.mode_toggle_interval}s bis Fullscreen ...")
        await self._wait(self.args.mode_toggle_interval)
        await self._toggle_view_mode()
        print(f"      {label}: bleibe {self.args.fullscreen_rest_sec}s im Fullscreen ...")
        await self._wait(self.args.fullscreen_rest_sec)
        await self._toggle_view_mode()

    async def _quick_call_action(self, label: str) -> None:
        coords = await self._update_click_coords()
        await self._click(coords["call_x"], coords["action_y"], f"({label})", fast=True)

    async def _quick_halfpot_raise_action(self, label: str) -> None:
        coords = await self._update_click_coords()
        await self._click(coords["half_pot_x"], coords["half_pot_y"], f"({label} 1/2-Pot)", fast=True)
        await asyncio.sleep(0.04)
        coords = await self._update_click_coords()
        await self._click(coords["raise_x"], coords["raise_active_y"], f"({label} Raise)", fast=True)

    async def _event_bootstrap(self) -> None:
        await self._wait_for_window()
        print("      Setze initialen Portrait-Modus ...")
        await self._apply_portrait_mode()
        c = await self._update_click_coords()
        print(f"      DEBUG: WX={self.wx} WY={self.wy}")
        print(f"             LokalesSpiel=({c['lokalgame_x']},{c['lokalgame_y']})")
        print(f"             SpielStarten=({c['spielstart_x']},{c['spielstart_y']})")

    async def _event_startshot(self) -> None:
        await self._shot("01_startseite.png")

    async def _event_open_localgame(self) -> None:
        c = await self._update_click_coords()
        await self._click(c["lokalgame_x"], c["lokalgame_y"], "(Lokales Spiel starten)")
        await asyncio.sleep(2)
        await self._shot("02_localgame_settings.png")

    async def _event_start_game(self) -> None:
        c = await self._update_click_coords()
        await self._click(c["spielstart_x"], c["spielstart_y"], "(Spiel starten)")
        print("      Warte auf GamePage (6s) ...")
        await asyncio.sleep(6)
        await self._shot("03_gamepage_preflop.png")
        await self._set_auto_check_fold_mode()
        await self._shot("03_gamepage_auto_mode.png")
        await self._start_periodic_log_overlay()

    async def _event_hand1(self) -> None:
        print("      Hand 1 - Spielverlauf ...")
        for idx in range(1, 5):
            await self._run_round_mode_cycle(f"Hand 1 Runde {idx}")
            await self._shot(f"04_hand1_runde{idx}.png")

    async def _event_hand2(self) -> None:
        print("      Hand 2 - Spielverlauf ...")
        await self._wait(self.args.hand_switch_delay)
        await self._shot("05_hand2_preflop.png")
        for idx in range(1, 4):
            await self._run_round_mode_cycle(f"Hand 2 Runde {idx}")
            await self._shot(f"05_hand2_runde{idx}.png")

        print("      Warte auf den Uebergang in Hand 3 ...")
        await self._wait(self.args.hand_switch_delay)
        await self._shot("05_hand3_start.png")

    async def _event_exit_to_start(self) -> None:
        await self._stop_periodic_log_overlay()
        print("      Zurueck zur Startseite ...")
        await self._wait(self.args.hand3_exit_delay)

        if self.current_view_mode != "portrait":
            print("      Wechsel zurueck in Portrait vor dem Exit ...")
            await self._apply_portrait_mode()

        print("      Escape (GamePage verlassen) ...")
        await self._run_cmd("xdotool", "key", "Escape", check=False)
        await asyncio.sleep(2)
        await self._shot("06_localgame_page_back.png")

        print("      Escape (LocalGamePage verlassen) ...")
        await self._run_cmd("xdotool", "key", "Escape", check=False)
        await asyncio.sleep(2)
        await self._shot("07_startseite_final.png")

    def _build_events(self) -> list[TimerEvent]:
        now = time.monotonic()
        events = [
            TimerEvent(now + 0.0, "bootstrap", self._event_bootstrap),
            TimerEvent(now + 0.2, "start-shot", self._event_startshot),
            TimerEvent(now + 0.4, "open-localgame", self._event_open_localgame),
            TimerEvent(now + 0.6, "start-game", self._event_start_game),
            TimerEvent(now + 0.8, "hand1", self._event_hand1),
            TimerEvent(now + 1.0, "hand2", self._event_hand2),
            TimerEvent(now + 1.2, "exit-to-start", self._event_exit_to_start),
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
        print("      Beende ffmpeg ...")
        self.ffmpeg_proc.send_signal(signal.SIGINT)
        try:
            await asyncio.wait_for(self.ffmpeg_proc.wait(), timeout=10)
        except TimeoutError:
            self.ffmpeg_proc.kill()
            await self.ffmpeg_proc.wait()
        self.ffmpeg_proc = None

    async def _final_summary(self) -> None:
        print("\n╔══════════════════════════════════════════════════╗")
        print("║   PokerTH QML - Lokales Spiel Demo - Fertig!    ║")
        print("╚══════════════════════════════════════════════════╝")
        print("\nScreenshots:")
        shots = sorted(self.output_dir.glob("*.png"))
        if not shots:
            print("  (keine)")
        else:
            for p in shots:
                print(f"  {p.name}")
        print()
        if self.video_file.exists():
            print(f"Video:  {self.video_file}")
        else:
            print(f"Video: nicht erstellt (Log: {self.ffmpeg_log})")

    async def cleanup(self) -> None:
        await self._stop_periodic_log_overlay()
        await self._stop_ffmpeg()

        for proc in (self.app_proc, self.wm_proc, self.xvfb_proc):
            if proc and proc.returncode is None:
                proc.terminate()

        for proc in (self.app_proc, self.wm_proc, self.xvfb_proc):
            if proc:
                with contextlib.suppress(Exception):
                    await asyncio.wait_for(proc.wait(), timeout=5)
                if proc.returncode is None:
                    with contextlib.suppress(Exception):
                        proc.kill()
                        await proc.wait()

        if self.audio_enabled:
            await self._run_cmd("pulseaudio", "--kill", check=False)

        if self.audio_runtime_dir:
            shutil.rmtree(self.audio_runtime_dir, ignore_errors=True)

        for handle in self._log_handles:
            with contextlib.suppress(Exception):
                handle.close()
        self._log_handles.clear()

    async def run(self) -> int:
        print(
            "[timing] profile="
            f"{self.args.timing_profile} mode_toggle={self.args.mode_toggle_interval}s "
            f"log_cycle={self.args.log_cycle_interval}s/{self.args.log_cycle_visible}s "
            f"fullscreen_rest={self.args.fullscreen_rest_sec}s "
            f"hand_switch={self.args.hand_switch_delay}s hand3_exit={self.args.hand3_exit_delay}s"
        )

        self.output_dir.mkdir(parents=True, exist_ok=True)
        for png in self.output_dir.glob("*.png"):
            png.unlink(missing_ok=True)

        if self.video_file.exists():
            self.video_file.unlink(missing_ok=True)

        if self.state_file.exists():
            self.state_file.unlink(missing_ok=True)

        log_printer = DeferredLogPrinter(
            log_files={"ffmpeg": self.ffmpeg_log, "qml-local": self.app_log},
            poll_interval=self.args.log_poll,
            batch_delay=self.args.log_delay,
        )

        try:
            await self._cleanup_strays()
            await self._start_services()
            self._log_task = asyncio.create_task(log_printer.run(self._log_stop))
            await self._run_event_loop()
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
    parser = argparse.ArgumentParser(description="Event-driven PokerTH QML Localgame Preview Recorder")
    parser.add_argument("--display-num", type=int, default=99)
    parser.add_argument("--desktop-w", type=int, default=1440)
    parser.add_argument("--desktop-h", type=int, default=900)
    parser.add_argument("--portrait-w", type=int, default=390)
    parser.add_argument("--portrait-h", type=int, default=844)
    parser.add_argument("--binary", default="/opt/pokerth_env/repos/pokerth-test/build/bin/pokerth_qml-client")
    parser.add_argument(
        "--timing-profile",
        choices=("fast", "normal", "slow"),
        default="normal",
        help="Vordefinierte Event-Timings fuer unterschiedliche Tischgeschwindigkeiten",
    )
    parser.add_argument("--mode-toggle-interval", type=float, default=10.0)
    parser.add_argument("--log-cycle-interval", type=float, default=12.0)
    parser.add_argument("--log-cycle-visible", type=float, default=4.0)
    parser.add_argument("--fullscreen-rest-sec", type=float, default=5.5)
    parser.add_argument("--hand-switch-delay", type=float, default=7.0)
    parser.add_argument("--hand3-exit-delay", type=float, default=4.0)
    parser.add_argument("--audio-sync-delay-ms", type=int, default=1200)
    parser.add_argument("--log-poll", type=float, default=0.8)
    parser.add_argument("--log-delay", type=float, default=1.5)
    return parser


def apply_timing_profile(args: argparse.Namespace) -> None:
    profile = TIMING_PROFILES.get(args.timing_profile)
    if not profile:
        return

    # Profilwerte zuerst setzen; explizit uebergebene CLI-Werte duerfen danach
    # weiterhin ueberschrieben werden, da argparse bereits geparst hat.
    for key, value in profile.items():
        setattr(args, key, value)

    cli = sys.argv[1:]
    overrides = {
        "mode_toggle_interval": "--mode-toggle-interval",
        "fullscreen_rest_sec": "--fullscreen-rest-sec",
        "hand_switch_delay": "--hand-switch-delay",
        "hand3_exit_delay": "--hand3-exit-delay",
    }
    for attr, flag in overrides.items():
        if flag in cli:
            idx = cli.index(flag)
            if idx + 1 < len(cli):
                setattr(args, attr, float(cli[idx + 1]))
            continue

        for arg in cli:
            if arg.startswith(flag + "="):
                setattr(args, attr, float(arg.split("=", 1)[1]))
                break


async def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    apply_timing_profile(args)
    recorder = LocalGameRecorder(args)
    return await recorder.run()


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))
