#!/usr/bin/env python3
"""
PokerTH QML-Client - Theme-Preview Generator

Schlanke Ableitung von record_pokerth_qml_localgame.py: startet den QML-Client
headless (Xvfb), navigiert ins lokale Spiel und erstellt ZWEI saubere Tisch-
Screenshots als Theme-Vorschauen fuer das QML-Spieltisch- und Karten-Theme:
  * preview.png          - Querformat/Desktop (Vollbild via F11)
  * preview_portrait.png - Portrait/Mobile (auf die Fenster-Region zugeschnitten)

Im Gegensatz zum vollen Localgame-Recorder wird KEIN Video/Audio aufgenommen und
es werden keine mehreren Haende durchgespielt - nur die beiden Tischvorschauen.

Ablauf: Startseite -> "Lokales Spiel starten" -> "Spiel starten" -> Tisch ->
        Portrait-Screenshot -> F11 (Vollbild) -> Querformat-Screenshot ->
        skalieren -> in die Theme-Verzeichnisse.

Benoetigte apt-Pakete:
  sudo apt install xvfb openbox scrot xdotool imagemagick

Beispiel:
  ./record_theme_preview.py
  ./record_theme_preview.py --display-num 95 --keep-fullscreen
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import time
from pathlib import Path


# Repo-Wurzel relativ zu diesem Skript (preview/ liegt direkt unter dem Repo).
REPO_ROOT = Path(__file__).resolve().parent.parent

# Klick-Koordinaten als Bruchteile der Fenster-Geometrie (Portrait 390x844),
# verifiziert gegen den aktuellen QML-Client-Stand. Identisch zu den Werten in
# record_pokerth_qml_localgame.py.
FRAC_LOCALGAME = (195 / 390, 405 / 844)   # "Lokales Spiel starten" (Startseite)
FRAC_STARTGAME = (288 / 390, 671 / 844)   # "Spiel starten" (Einstellungsseite)


class ThemePreviewRecorder:
    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.script_dir = Path(__file__).resolve().parent
        self.display = f":{args.display_num}"
        self.display_res = f"{args.desktop_w}x{args.desktop_h}"
        self.binary = Path(args.binary)

        self.env = {**__import__("os").environ, "DISPLAY": self.display}

        self.xvfb: subprocess.Popen | None = None
        self.wm: subprocess.Popen | None = None
        self.app: subprocess.Popen | None = None
        self.win_id: str | None = None

        # Ziele fuer die Vorschauen (preview/ + beide Theme-Ordner).
        # Querformat -> preview.png, Portrait/Mobile -> preview_portrait.png
        self.targets_landscape = [
            self.script_dir / "theme_preview.png",
            REPO_ROOT / "data/gfx/qml/table/default/preview.png",
            REPO_ROOT / "data/gfx/qml/cards/default/preview.png",
        ]
        self.targets_portrait = [
            self.script_dir / "theme_preview_portrait.png",
            REPO_ROOT / "data/gfx/qml/table/default/preview_portrait.png",
            REPO_ROOT / "data/gfx/qml/cards/default/preview_portrait.png",
        ]

    # ── Hilfen ────────────────────────────────────────────────────────────────
    def _run(self, *cmd: str, check: bool = True) -> str:
        res = subprocess.run(cmd, env=self.env, capture_output=True, text=True)
        if check and res.returncode != 0:
            raise RuntimeError(f"Befehl fehlgeschlagen: {' '.join(cmd)}\n{res.stderr.strip()}")
        return res.stdout

    def _start(self, *cmd: str) -> subprocess.Popen:
        return subprocess.Popen(cmd, env=self.env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def _geom(self) -> dict[str, int]:
        out = self._run("xdotool", "getwindowgeometry", "--shell", self.win_id, check=False)
        vals: dict[str, int] = {}
        for line in out.splitlines():
            if "=" in line:
                k, v = line.split("=", 1)
                if v.strip().lstrip("-").isdigit():
                    vals[k.strip()] = int(v.strip())
        return vals

    def _click_frac(self, frac: tuple[float, float], desc: str) -> None:
        g = self._geom()
        x = g.get("X", 0) + int(g.get("WIDTH", self.args.portrait_w) * frac[0])
        y = g.get("Y", 0) + int(g.get("HEIGHT", self.args.portrait_h) * frac[1])
        print(f"      Klick ({x},{y}) {desc}")
        self._run("xdotool", "mousemove", "--sync", str(x), str(y))
        time.sleep(0.2)
        self._run("xdotool", "click", "--clearmodifiers", "1")

    # ── Phasen ──────────────────────────────────────────────────────────────--
    def _start_services(self) -> None:
        print(f"[1/5] Starte Xvfb {self.display} ({self.display_res}x24) ...")
        self.xvfb = self._start("Xvfb", self.display, "-screen", "0", f"{self.display_res}x24", "-ac")
        time.sleep(2.0)
        print("[2/5] Starte openbox ...")
        self.wm = self._start("openbox")
        time.sleep(1.0)
        print("[3/5] Starte QML-Client ...")
        self.app = self._start(str(self.binary))

    def _wait_for_window(self) -> None:
        print("      Warte auf QML-Fenster ...")
        for _ in range(40):
            out = self._run("xdotool", "search", "--onlyvisible", "--name", "PokerTH", check=False)
            win = next((l.strip() for l in out.splitlines() if l.strip()), "")
            if win:
                self.win_id = win
                break
            time.sleep(1.0)
        if not self.win_id:
            raise RuntimeError("QML-Fenster nicht gefunden")

        # Portrait-Fenster zentrieren - definierte Geometrie fuer die Klicks.
        px = (self.args.desktop_w - self.args.portrait_w) // 2
        py = (self.args.desktop_h - self.args.portrait_h) // 2
        self._run("xdotool", "windowsize", "--sync", self.win_id, str(self.args.portrait_w), str(self.args.portrait_h), check=False)
        self._run("xdotool", "windowmove", "--sync", self.win_id, str(px), str(py), check=False)
        self._run("xdotool", "windowfocus", self.win_id, check=False)
        print(f"      Fenster {self.win_id} - warte {self.args.preloader_sec}s (PreLoader) ...")
        time.sleep(self.args.preloader_sec)

    def _navigate_to_table(self) -> None:
        print("[4/6] Navigiere ins lokale Spiel ...")
        self._click_frac(FRAC_LOCALGAME, "(Lokales Spiel starten)")
        time.sleep(2.5)
        self._click_frac(FRAC_STARTGAME, "(Spiel starten)")
        print(f"      Warte {self.args.table_sec}s auf den Spieltisch ...")
        time.sleep(self.args.table_sec)

    def _write_scaled(self, raw: Path, targets: list[Path], size: str,
                      crop: str | None = None) -> None:
        magick = shutil.which("magick") or shutil.which("convert")
        for dst in targets:
            dst.parent.mkdir(parents=True, exist_ok=True)
            if magick:
                cmd = [magick, str(raw)]
                if crop:
                    cmd += ["-crop", crop, "+repage"]
                cmd += ["-resize", size, "-strip", str(dst)]
                subprocess.run(cmd, check=True)
            else:
                shutil.copyfile(raw, dst)  # Fallback: unskalierte Kopie
            print(f"      -> {dst}")

    def _capture_portrait(self) -> None:
        # Portrait/Mobile: Tisch im Hochformat-Fenster, auf Fenster-Region zugeschnitten.
        print("[5/6] Erstelle Portrait-Vorschau ...")
        g = self._geom()
        crop = f"{g['WIDTH']}x{g['HEIGHT']}+{g['X']}+{g['Y']}"
        raw = self.script_dir / "_theme_preview_raw.png"
        self._run("scrot", str(raw))
        self._write_scaled(raw, self.targets_portrait,
                           f"{self.args.portrait_preview_w}x{self.args.portrait_preview_h}",
                           crop=crop)
        raw.unlink(missing_ok=True)

    def _capture_landscape(self) -> None:
        # Querformat: per F11 in den Vollbild-/Breitformat-Tisch wechseln.
        print("[6/6] Erstelle Querformat-Vorschau ...")
        self._run("xdotool", "windowactivate", "--sync", self.win_id, check=False)
        self._run("xdotool", "key", "--clearmodifiers", "F11", check=False)
        time.sleep(2.5)
        raw = self.script_dir / "_theme_preview_raw.png"
        self._run("scrot", str(raw))
        self._write_scaled(raw, self.targets_landscape,
                           f"{self.args.preview_w}x{self.args.preview_h}")
        raw.unlink(missing_ok=True)

    def cleanup(self) -> None:
        for proc in (self.app, self.wm, self.xvfb):
            if proc and proc.poll() is None:
                proc.terminate()
        for proc in (self.app, self.wm, self.xvfb):
            if proc:
                try:
                    proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.kill()

    def run(self) -> int:
        try:
            self._start_services()
            self._wait_for_window()
            self._navigate_to_table()
            self._capture_portrait()
            self._capture_landscape()
            print("\nFertig - Theme-Vorschauen (Portrait + Querformat) erstellt.")
            return 0
        except Exception as exc:  # noqa: BLE001
            print(f"[FEHLER] {exc}", file=sys.stderr)
            return 1
        finally:
            self.cleanup()


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="PokerTH QML Theme-Preview Generator")
    p.add_argument("--display-num", type=int, default=95)
    p.add_argument("--desktop-w", type=int, default=1440)
    p.add_argument("--desktop-h", type=int, default=900)
    p.add_argument("--portrait-w", type=int, default=390)
    p.add_argument("--portrait-h", type=int, default=844)
    p.add_argument("--preview-w", type=int, default=800)
    p.add_argument("--preview-h", type=int, default=500)
    p.add_argument("--portrait-preview-w", type=int, default=380)
    p.add_argument("--portrait-preview-h", type=int, default=822)
    p.add_argument("--preloader-sec", type=float, default=9.0)
    p.add_argument("--table-sec", type=float, default=7.0)
    p.add_argument("--binary", default=str(REPO_ROOT / "build/bin/pokerth_qml-client"))
    return p


def main() -> int:
    return ThemePreviewRecorder(build_parser().parse_args()).run()


if __name__ == "__main__":
    raise SystemExit(main())
