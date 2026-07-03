#!/usr/bin/env python3
"""
PokerTH QML Online-Preview (event-driven)
Flow: Startseite -> Internet-Login als Benutzer -> Lobby -> Chat+Emoji (wide)
    -> Chat+Emoji (fullscreen) -> Spiel beitreten.

Default-Verhalten: Der Ablauf endet nach dem Lobby-Join-Schritt.
Optional kann mit --continue-after-join der Ingame-Teil aktiviert werden.

Internet-Server wird manuell gesetzt auf:
    Host pthsrv.pokerth.net, Port 7234, TLS aus

Benoetigte apt-Pakete:
  sudo apt install xvfb openbox ffmpeg scrot xdotool
"""

from __future__ import annotations

import argparse
import asyncio
import base64
import contextlib
import heapq
import os
import random
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


class OnlineChatRecorder:
    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.script_dir = Path(__file__).resolve().parent
        self.output_dir = self.script_dir / "screenshots_online_chat"
        self.video_file = self.script_dir / "pokerth_qml_online_chat_demo.mp4"
        self.ffmpeg_log = self.script_dir / "ffmpeg_online_chat.log"
        self.app_log = self.script_dir / "pokerth_online_chat.log"

        self.display = f":{args.display_num}"
        self.display_res = args.display_res
        self.binary = Path(args.binary)

        m = re.match(r"^(\d+)x(\d+)$", self.display_res)
        if not m:
            raise ValueError(f"Ungueltige display-res: {self.display_res}")
        self.desktop_w = int(m.group(1))
        self.desktop_h = int(m.group(2))

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
        self.ww = 390
        self.wh = 844
        self.fullscreen = False

        self._log_stop = asyncio.Event()
        self._log_task: asyncio.Task[None] | None = None
        self._action_idx = 0
        self._rng = random.Random(self.args.emoji_seed)

    async def _read_app_log(self) -> str:
        if not self.app_log.exists():
            return ""
        return self.app_log.read_text(encoding="utf-8", errors="replace")

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
        for pattern in ("pokerth_qml-client", f"Xvfb :{self.args.display_num}", "openbox"):
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

        print("[4/6] Starte QML-Client ...")
        await self._configure_internet_settings()
        self.app_proc = await self._start_proc(str(self.binary), log_file=self.app_log)

    async def _configure_internet_settings(self) -> None:
        cfg = Path.home() / ".pokerth" / "config.xml"
        if not cfg.exists():
            print("      Hinweis: ~/.pokerth/config.xml nicht gefunden - Internet-Settings bleiben unveraendert.")
            return

        tls_value = "1" if self.args.internet_tls == "on" else "0"
        print(
            "      Setze manuelle Internet-Settings: "
            f"{self.args.internet_server}:{self.args.internet_port}, tls={self.args.internet_tls}"
        )
        encoded_password = base64.b64encode(self.args.login_password.encode("utf-8")).decode("ascii")

        # ConfigMode 1 = manueller Server statt Serverliste.
        replacements = [
            ('s|<InternetLoginMode value="[0-9]*"/>|<InternetLoginMode value="0"/>|', "InternetLoginMode"),
            ('s|<InternetServerConfigMode value="[0-9]*"/>|<InternetServerConfigMode value="1"/>|', "InternetServerConfigMode"),
            (f's|<InternetServerAddress value="[^"]*"/>|<InternetServerAddress value="{self.args.internet_server}"/>|', "InternetServerAddress"),
            (f's|<InternetServerPort value="[0-9]*"/>|<InternetServerPort value="{self.args.internet_port}"/>|', "InternetServerPort"),
            (f's|<InternetServerUseTls value="[0-9]*"/>|<InternetServerUseTls value="{tls_value}"/>|', "InternetServerUseTls"),
            (f's|<MyName value="[^"]*"/>|<MyName value="{self.args.login_user}"/>|', "MyName"),
            ('s|<InternetSavePassword value="[0-9]*"/>|<InternetSavePassword value="1"/>|', "InternetSavePassword"),
            (f's|<InternetLoginPassword value="[^"]*"/>|<InternetLoginPassword value="{encoded_password}"/>|', "InternetLoginPassword"),
        ]
        for sed_expr, tag in replacements:
            await self._run_cmd("sed", "-i", sed_expr, str(cfg), check=False)
            grep_out = await self._run_cmd("grep", "-n", f"<{tag}", str(cfg), check=False)
            if grep_out.strip():
                print(f"      {grep_out.strip()}")

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
            await self._run_cmd("scrot", str(self.output_dir / "debug_no_window.png"), check=False)
            raise RuntimeError("QML-Fenster nicht gefunden")

        await self._refresh_window_geometry()
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

    async def _shot(self, name: str, pause: float = 0.8) -> None:
        path = self.output_dir / name
        await asyncio.sleep(pause)
        await self._run_cmd("scrot", "-p", str(path))
        print(f"      Screenshot -> {path}")

    def _slug(self, text: str) -> str:
        return re.sub(r"[^a-z0-9]+", "_", text.lower()).strip("_")[:40] or "step"

    async def _step_shot_before_after(self, label: str, runner: Callable[[], Awaitable[None]]) -> None:
        self._action_idx += 1
        idx = self._action_idx
        slug = self._slug(label)
        await self._shot(f"step_{idx:03d}_{slug}_before.png", pause=0.08)
        await runner()
        await self._shot(f"step_{idx:03d}_{slug}_after.png", pause=0.08)

    async def _focus_window(self) -> None:
        if not self.win_id:
            return
        await self._run_cmd("xdotool", "windowactivate", "--sync", self.win_id, check=False)
        await asyncio.sleep(0.05)

    async def _click(self, x: int, y: int, desc: str = "") -> None:
        if self.win_id:
            await self._refresh_window_geometry()
            inside = self.wx <= x <= (self.wx + self.ww) and self.wy <= y <= (self.wy + self.wh)
            if not inside:
                raise RuntimeError(f"Falscher Klick erkannt (außerhalb Fenster): ({x},{y})")

        async def _do_click() -> None:
            print(f"      Klick ({x}, {y}) {desc}")
            await self._focus_window()
            await self._run_cmd("xdotool", "mousemove", "--sync", str(x), str(y), check=False)
            await asyncio.sleep(0.2)
            await self._run_cmd("xdotool", "click", "--clearmodifiers", "1", check=False)

        await self._step_shot_before_after(f"click_{desc or f'{x}_{y}'}", _do_click)

    async def _type(self, text: str) -> None:
        async def _do_type() -> None:
            await self._focus_window()
            await self._run_cmd("xdotool", "type", "--clearmodifiers", "--delay", "1", text, check=False)

        await self._step_shot_before_after("type_text", _do_type)

    async def _key(self, *keys: str) -> None:
        async def _do_key() -> None:
            await self._focus_window()
            await self._run_cmd("xdotool", "key", *keys, check=False)

        await self._step_shot_before_after(f"key_{'_'.join(keys)}", _do_key)

    async def _apply_wide_window(self) -> None:
        if not self.win_id:
            return
        if self.fullscreen:
            await self._key("F11")
            await asyncio.sleep(0.4)
            self.fullscreen = False

        px = max(0, (self.desktop_w - self.args.wide_width) // 2)
        py = max(0, (self.desktop_h - self.args.wide_height) // 2)
        await self._run_cmd("xdotool", "windowsize", "--sync", self.win_id, str(self.args.wide_width), str(self.args.wide_height), check=False)
        await self._run_cmd("xdotool", "windowmove", "--sync", self.win_id, str(px), str(py), check=False)
        await self._run_cmd("xdotool", "windowfocus", self.win_id, check=False)
        await asyncio.sleep(0.5)
        await self._refresh_window_geometry()

    async def _toggle_fullscreen(self, on: bool) -> None:
        if self.fullscreen == on:
            return
        await self._run_cmd("xdotool", "windowactivate", "--sync", self.win_id or "", check=False)
        await self._key("F11")
        await asyncio.sleep(0.8)
        self.fullscreen = on
        await self._refresh_window_geometry()

    def _coords_compact_login(self) -> tuple[int, int, int, int]:
        internet_x = self.wx + 195
        internet_y = self.wy + 349
        guest_x = self.wx + 195
        guest_y = self.wy + 507
        return internet_x, internet_y, guest_x, guest_y

    def _coords_user_login_dialog(self) -> dict[str, int]:
        # Koordinaten aus dem QML-Layout hergeleitet (ServerConnectionDialog.qml),
        # damit Klicks trotz Fenster-Offsets reproduzierbar auf den Controls landen.
        card_w = min(int(self.ww * 0.9), 360)
        card_h = min(int(self.wh * 0.88), 500)
        card_x = self.wx + (self.ww - card_w) // 2
        card_y = self.wy + (self.wh - card_h) // 2

        stack_margin = 28
        stack_spacing = 12
        row_spacing = 12
        field_h = 46
        label_h = 28
        line_h = 1
        checkbox_h = 28
        row_h = 46

        stack_x = card_x + stack_margin
        stack_y = card_y + stack_margin
        stack_w = card_w - (2 * stack_margin)
        stack_h = card_h - (2 * stack_margin)

        # View 1 (Login-Formular): top-fill + feste Elemente + bottom-fill
        fixed_h = (
            label_h
            + line_h
            + field_h
            + field_h
            + checkbox_h
            + row_h
            + (6 * stack_spacing)
        )
        top_fill = max(0, (stack_h - fixed_h) // 2)

        y_cursor = stack_y + top_fill
        y_cursor += label_h + stack_spacing + line_h + stack_spacing
        username_y = y_cursor + (field_h // 2)
        y_cursor += field_h + stack_spacing
        password_y = y_cursor + (field_h // 2)
        y_cursor += field_h + stack_spacing + checkbox_h + stack_spacing
        login_row_y = y_cursor + (row_h // 2)

        # Rechte Button-Hälfte in der RowLayout (Back | Login)
        login_x = stack_x + int(0.75 * stack_w) + int(0.25 * row_spacing)

        # View 0 (Auswahlseite): fill + 3 Buttons + fill, spacing=18
        choice_spacing = 18
        choice_fixed_h = (3 * field_h) + (4 * choice_spacing)
        choice_top_fill = max(0, (stack_h - choice_fixed_h) // 2)
        login_user_y = stack_y + choice_top_fill + (field_h // 2)

        center_x = stack_x + (stack_w // 2)
        return {
            "login_user_btn_x": center_x,
            "login_user_btn_y": login_user_y,
            "username_x": center_x,
            "username_y": username_y,
            "password_x": center_x,
            "password_y": password_y,
            "login_btn_x": login_x,
            "login_btn_y": login_row_y,
        }

    def _coords_wide_chat(self) -> dict[str, int]:
        # Empirisch auf die reale Wide-Lobby kalibriert.
        # Die Chat-Row liegt deutlich über der unteren Fensterkante.
        row_y = self.wy + self.wh - 78
        emoji_btn_x = self.wx + int(self.ww * 0.57)
        chat_input_x = self.wx + int(self.ww * 0.78)
        send_btn_x = self.wx + int(self.ww * 0.96)
        return {
            "emoji_btn_x": emoji_btn_x,
            "chat_input_x": chat_input_x,
            "send_btn_x": send_btn_x,
            "chat_row_y": row_y,
            "emoji_pick_x": emoji_btn_x + 28,
            "emoji_pick_y": row_y - 120,
            # Wide-Layout: erste Zeile in der mittleren Spielliste.
            "game_item_x": self.wx + int(self.ww * 0.42),
            "game_item_y": self.wy + int(self.wh * 0.16),
            # Join-Button sitzt in der unteren Action-Row rechts vom "Create Game"-Button.
            "join_btn_x": self.wx + int(self.ww * 0.56),
            "join_btn_y": self.wy + self.wh - 68,
        }

    def _coords_game_chat_overlay(self) -> dict[str, int]:
        # GamePage: Chat-Overlay links, Breite ~1/3 (mind. 300), Chatzeile unten.
        overlay_w = max(self.ww // 3, 300)
        panel_left = self.wx + 10
        panel_top = self.wy + 40 + 50 + 10  # Statusbar(40) + overlay top margin + panel margin
        panel_bottom = self.wy + self.wh - 10
        row_y = panel_bottom - 12 - 18

        return {
            "chat_toggle_x": self.wx + 25,
            "chat_toggle_y": self.wy + 65,
            "emoji_btn_x": panel_left + 18,
            "chat_input_x": panel_left + min(170, max(120, overlay_w // 2)),
            "send_btn_x": panel_left + min(overlay_w - 40, 300),
            "chat_row_y": row_y,
            "emoji_pick_x": panel_left + 18,
            "emoji_pick_y": panel_top + max(110, (panel_bottom - panel_top) - 230),
        }

    async def _send_chat_with_emoji(self, prefix: str) -> None:
        c = self._coords_wide_chat()
        await self._click(c["chat_input_x"], c["chat_row_y"], f"({prefix} Chatfeld)")
        await self._type(f"{prefix}: ready ")
        await self._click(c["emoji_btn_x"], c["chat_row_y"], f"({prefix} Emoji-Button)")
        await asyncio.sleep(0.4)
        await self._click(c["emoji_pick_x"], c["emoji_pick_y"], f"({prefix} Emoji wählen)")
        await asyncio.sleep(0.2)
        await self._click(c["send_btn_x"], c["chat_row_y"], f"({prefix} Senden)")
        await asyncio.sleep(0.7)

    def _random_emoji_points(self, base_x: int, base_y: int, count: int) -> list[tuple[int, int]]:
        # Picker ist eine kleine Grid-Fläche links über dem Chat-Input.
        # Random-Klicks bleiben bewusst innerhalb dieser Box.
        points: list[tuple[int, int]] = []
        min_x = max(self.wx + 12, base_x - 18)
        max_x = min(self.wx + self.ww - 12, base_x + 120)
        min_y = max(self.wy + 12, base_y - 70)
        max_y = min(self.wy + self.wh - 12, base_y + 42)
        for _ in range(count):
            px = self._rng.randint(min_x, max_x)
            py = self._rng.randint(min_y, max_y)
            points.append((px, py))
        return points

    async def _send_chat_with_random_emojis(self, prefix: str, emoji_clicks: int) -> None:
        c = self._coords_wide_chat()
        await self._click(c["chat_input_x"], c["chat_row_y"], f"({prefix} Chatfeld)")
        await self._key("ctrl+a")
        await self._type(f"{prefix}: ")
        for i, (ex, ey) in enumerate(self._random_emoji_points(c["emoji_pick_x"], c["emoji_pick_y"], emoji_clicks), start=1):
            await self._click(c["emoji_btn_x"], c["chat_row_y"], f"({prefix} Emoji-Button {i})")
            await asyncio.sleep(0.45)
            await self._click(ex, ey, f"({prefix} Random-Emoji {i})")
            await asyncio.sleep(0.15)
        await self._click(c["send_btn_x"], c["chat_row_y"], f"({prefix} Senden)")
        await asyncio.sleep(0.8)

    async def _send_game_chat_with_emoji(self, prefix: str) -> None:
        c = self._coords_game_chat_overlay()
        # Alt+C toggelt den Game-Chat robust, unabhängig von exakter Toggle-Position.
        await self._key("alt+c")
        await asyncio.sleep(0.6)
        await self._click(c["chat_input_x"], c["chat_row_y"], f"({prefix} Game-Chatfeld)")
        await self._type(f"{prefix}: gl hf ")
        await self._click(c["emoji_btn_x"], c["chat_row_y"], f"({prefix} Game-Emoji-Button)")
        await asyncio.sleep(0.4)
        await self._click(c["emoji_pick_x"], c["emoji_pick_y"], f"({prefix} Game-Emoji waehlen)")
        await asyncio.sleep(0.2)
        await self._key("Return")
        await asyncio.sleep(0.8)
        await self._key("alt+c")  # Overlay wieder schließen
        await asyncio.sleep(0.4)

    async def _join_first_game(self) -> None:
        c = self._coords_wide_chat()
        await self._focus_window()
        print(f"      Klick ({c['game_item_x']}, {c['game_item_y']}) (erstes Spiel auswählen Doppelklick)")
        await self._run_cmd("xdotool", "mousemove", "--sync", str(c["game_item_x"]), str(c["game_item_y"]), check=False)
        await asyncio.sleep(0.15)
        await self._run_cmd("xdotool", "click", "--clearmodifiers", "--repeat", "2", "--delay", "120", "1", check=False)
        await asyncio.sleep(0.6)
        await self._click(c["game_item_x"], c["game_item_y"] + 26, "(Spielzeile bestätigen)")
        await asyncio.sleep(0.3)
        await self._click(c["join_btn_x"], c["join_btn_y"], "(Join Game)")

    async def _wait_for_any_log_marker(self, markers: tuple[str, ...], timeout: float) -> bool:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            log_text = await self._read_app_log()
            if any(m in log_text for m in markers):
                return True
            await asyncio.sleep(0.25)
        return False

    async def _event_login_user(self) -> None:
        internet_x, internet_y, _, _ = self._coords_compact_login()
        c = self._coords_user_login_dialog()
        await self._shot("01_startseite.png")
        await self._click(internet_x, internet_y, "(Internetspiel)")
        await asyncio.sleep(2.0)
        await self._shot("02_login.png")
        await self._click(c["login_user_btn_x"], c["login_user_btn_y"], "(Login as User)")
        await asyncio.sleep(1.0)
        await self._click(c["username_x"], c["username_y"], "(Username)")
        await self._key("ctrl+a")
        await self._type(self.args.login_user)
        await self._key("Tab")
        await self._key("ctrl+a")
        await self._type(self.args.login_password)

        # Mehrfachklick auf den Login-Button mit kleinem Y-Offset als robuster Submit.
        await self._click(c["login_btn_x"], c["login_btn_y"], "(Login Klick 1)")
        await asyncio.sleep(0.4)
        await self._click(c["login_btn_x"], c["login_btn_y"] - 10, "(Login Klick 2)")
        await asyncio.sleep(0.4)
        await self._click(c["login_btn_x"], c["login_btn_y"] + 10, "(Login Klick 3)")
        await asyncio.sleep(5.0)
        await self._shot("03_lobby_compact.png")

    async def _event_wide_chat(self) -> None:
        await self._apply_wide_window()
        await self._shot("04_lobby_wide.png")
        await self._send_chat_with_emoji("wide")
        await self._shot("05_chat_wide_emoji.png")

    async def _event_fullscreen_chat(self) -> None:
        await self._toggle_fullscreen(True)
        await self._shot("06_lobby_fullscreen.png")
        await self._send_chat_with_emoji("fullscreen")
        await self._shot("07_chat_fullscreen_emoji.png")

    async def _event_extra_messages_random_emojis(self) -> None:
        await self._apply_wide_window()
        await self._shot("07b_extra_before.png")
        await self._send_chat_with_random_emojis("extra-msg-1", emoji_clicks=3)
        await self._shot("07c_extra_msg1_sent.png")
        await self._send_chat_with_random_emojis("extra-msg-2", emoji_clicks=3)
        await self._shot("07d_extra_msg2_sent.png")
        print(f"      Warte {self.args.after_second_message_wait}s bis Start durch 2. Client ...")
        await asyncio.sleep(self.args.after_second_message_wait)
        await self._shot("07e_after_second_msg_wait.png")

    async def _event_join_and_wait_start(self) -> None:
        await self._apply_wide_window()
        await self._join_first_game()
        joined = await self._wait_for_any_log_marker(
            markers=("[NAV] onSelfJoinedGame", "pushing GameWaitPage"),
            timeout=self.args.join_detect_timeout,
        )
        if not joined:
            await self._shot("08_join_failed.png")
            raise RuntimeError("Join nicht erkannt: kein Wechsel nach GameWaitPage")
        await asyncio.sleep(self.args.host_start_wait)
        await self._shot("08_joined_wait_or_start.png")

    async def _wait_for_game_start(self) -> None:
        print(f"      Warte auf Spielstart durch 2. Client (max {self.args.game_start_timeout}s) ...")
        deadline = time.monotonic() + self.args.game_start_timeout
        markers = (
            "GameWaitPage.onGameStarted",
            "onGameStarted",
            "pushing GamePage",
            "GamePage",
        )
        while time.monotonic() < deadline:
            log_text = await self._read_app_log()
            if any(m in log_text for m in markers):
                print("      Spielstart erkannt (Log-Marker).")
                await asyncio.sleep(1.0)
                return
            await asyncio.sleep(0.5)
        print("      Kein eindeutiger Marker gefunden - fahre mit Game-Chat-Versuch fort.")

    async def _event_game_chat_wide(self) -> None:
        await self._wait_for_game_start()
        await self._apply_wide_window()
        await self._shot("09_game_wide.png")
        await self._send_game_chat_with_emoji("game-wide")
        await self._shot("10_game_chat_wide_emoji.png")

    async def _event_game_chat_fullscreen(self) -> None:
        await self._toggle_fullscreen(True)
        await self._shot("11_game_fullscreen.png")
        await self._send_game_chat_with_emoji("game-fullscreen")
        await self._shot("12_game_chat_fullscreen_emoji.png")

    def _build_events(self) -> list[TimerEvent]:
        now = time.monotonic()
        events = [
            TimerEvent(now + 0.0, "login-user", self._event_login_user),
            TimerEvent(now + 0.2, "wide-chat", self._event_wide_chat),
            TimerEvent(now + 0.4, "fullscreen-chat", self._event_fullscreen_chat),
            TimerEvent(now + 0.6, "extra-messages-emojis", self._event_extra_messages_random_emojis),
            TimerEvent(now + 0.8, "join-game", self._event_join_and_wait_start),
        ]
        if self.args.continue_after_join:
            events.extend(
                [
                    TimerEvent(now + 1.0, "game-chat-wide", self._event_game_chat_wide),
                    TimerEvent(now + 1.2, "game-chat-fullscreen", self._event_game_chat_fullscreen),
                ]
            )
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
            try:
                await ev.action()
            except Exception:
                with contextlib.suppress(Exception):
                    await self._shot(f"error_{ev.name}.png")
                raise

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
        print("\n╔══════════════════════════════════════════════╗")
        print("║   PokerTH QML Online Chat Demo - Fertig     ║")
        print("╚══════════════════════════════════════════════╝")
        print("\nScreenshots:")
        for p in sorted(self.output_dir.glob("*.png")):
            print(f"  {p.name}")
        if self.video_file.exists():
            print(f"\nVideo: {self.video_file}")
        else:
            print(f"\nVideo: nicht erstellt (Log: {self.ffmpeg_log})")

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
            log_files={"ffmpeg": self.ffmpeg_log, "qml-online": self.app_log},
            poll_interval=self.args.log_poll,
            batch_delay=self.args.log_delay,
        )

        try:
            await self._cleanup_strays()
            await self._start_services()
            self._log_task = asyncio.create_task(log_printer.run(self._log_stop))
            await self._wait_for_window()
            await self._run_event_loop()
            await asyncio.sleep(self.args.final_hold)
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
    parser = argparse.ArgumentParser(description="Event-driven PokerTH QML Online Chat Recorder")
    parser.add_argument("--display-num", type=int, default=98)
    parser.add_argument("--display-res", default="1440x900")
    parser.add_argument("--binary", default="/opt/pokerth_env/repos/pokerth-test/build/bin/pokerth_qml-client")
    parser.add_argument("--login-user", default="sp0ck")
    parser.add_argument("--login-password", default="m4551m05")
    parser.add_argument("--internet-server", default="pthsrv.pokerth.net")
    parser.add_argument("--internet-port", type=int, default=7234)
    parser.add_argument("--internet-tls", choices=("off", "on"), default="off")
    parser.add_argument("--wide-width", type=int, default=1200)
    parser.add_argument("--wide-height", type=int, default=820)
    parser.add_argument("--host-start-wait", type=float, default=3.0, help="Wartezeit in Sekunden auf Start durch zweiten Client")
    parser.add_argument("--after-second-message-wait", type=float, default=10.0, help="Wartezeit nach zweiter Zusatznachricht")
    parser.add_argument("--join-detect-timeout", type=float, default=6.0, help="Maximale Wartezeit auf Log-Marker fuer Game-Join")
    parser.add_argument("--game-start-timeout", type=float, default=18.0, help="Maximale Wartezeit bis GamePage nach Join")
    parser.add_argument("--continue-after-join", action="store_true", help="Fuehrt nach Lobby-Join auch den Ingame-Teil aus")
    parser.add_argument("--emoji-seed", type=int, default=20260525, help="Seed fuer reproduzierbare Random-Emoji-Klicks")
    parser.add_argument("--final-hold", type=float, default=1.0)
    parser.add_argument("--log-poll", type=float, default=0.8)
    parser.add_argument("--log-delay", type=float, default=1.5)
    return parser


async def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    recorder = OnlineChatRecorder(args)
    return await recorder.run()


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))
