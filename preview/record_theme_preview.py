#!/usr/bin/env python3
"""
PokerTH QML-Client - Theme-Preview Generator

Schlanke Ableitung von record_pokerth_qml_localgame.py: startet den QML-Client
headless (Xvfb), navigiert ins lokale Spiel und erstellt ZWEI saubere Tisch-
Screenshots als Theme-Vorschauen fuer das QML-SPIELTISCH-Theme:
  * preview.png          - Querformat/Desktop (Vollbild via F11)
  * preview_portrait.png - Portrait/Mobile (auf die Fenster-Region zugeschnitten)

Die Kartenstapel- und Kartenrueckseiten-Vorschauen werden NICHT hier erzeugt,
sondern direkt aus den SVGs gebaut: preview/build_card_previews.sh

Im Gegensatz zum vollen Localgame-Recorder wird KEIN Video/Audio aufgenommen und
es werden keine mehreren Haende durchgespielt - nur die beiden Tischvorschauen.

Ablauf: Startseite -> "Lokales Spiel starten" -> "Spiel starten" -> Tisch ->
        Call (F2) + warten bis der Flop liegt (--preflop schaltet das ab) ->
        Bet (F3), damit Einsaetze in den Boxen stehen (--no-flop-bet aus) ->
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
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path


# Repo-Wurzel relativ zu diesem Skript (preview/ liegt direkt unter dem Repo).
REPO_ROOT = Path(__file__).resolve().parent.parent

# Klick-Koordinaten als Bruchteile der Fenster-Geometrie (Portrait 390x844),
# verifiziert gegen den aktuellen QML-Client-Stand. Die Startseite hat seit
# "Netzwerkspiel erstellen" eine Schaltflaeche mehr, deshalb liegt der lokale
# Spielstart hoeher als in record_pokerth_qml_localgame.py (dort noch 405/844).
FRAC_LOCALGAME = (195 / 390, 358 / 844)   # "Lokales Spiel starten" (Startseite)
FRAC_STARTGAME = (288 / 390, 671 / 844)   # "Spiel starten" (Einstellungsseite)

# Bildausschnitte, an denen abgelesen wird, ob eine Tastenaktion angekommen ist -
# als Bruchteile (x0, y0, x1, y1) der Fenster-Geometrie. Beide Bereiche legt der
# Client selbst an (Kartenreihe mittig, Kopfzeile oben), das Tisch-Thema tauscht
# nur den Hintergrund: die Rechtecke gelten also fuer JEDES Thema.
FRAC_BOARD  = (0.18, 0.42, 0.79, 0.49)   # Reihe der Gemeinschaftskarten (ohne Pot-Chip)
FRAC_HEADER = (0.02, 0.02, 0.32, 0.08)   # "Gesamt:" / "Einsaetze:" links oben


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

        # Ziele fuer die Vorschauen (preview/ + Spieltisch-Theme-Ordner).
        # Querformat -> preview.png, Portrait/Mobile -> preview_portrait.png.
        # NUR der Spieltisch-Stil bekommt seine Vorschau aus diesem Screenshot.
        # Kartenstapel- und Kartenrueckseiten-Vorschauen werden NICHT aus einem
        # Tisch-Screenshot, sondern direkt aus den SVGs gebaut:
        #   preview/build_card_previews.sh
        style_dir = REPO_ROOT / "data/gfx/qml/table" / args.style
        self.targets_landscape = [
            self.script_dir / "theme_preview.png",
            style_dir / "preview.png",
        ]
        self.targets_portrait = [
            self.script_dir / "theme_preview_portrait.png",
            style_dir / "preview_portrait.png",
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

    def _grab_region(self, frac: tuple[float, float, float, float], dst: Path) -> None:
        # Fenster-Ausschnitt als PNG - Grundlage fuer den Vorher/Nachher-Vergleich.
        g = self._geom()
        x0 = g.get("X", 0) + int(g.get("WIDTH", self.args.portrait_w) * frac[0])
        y0 = g.get("Y", 0) + int(g.get("HEIGHT", self.args.portrait_h) * frac[1])
        w = max(1, int(g.get("WIDTH", self.args.portrait_w) * (frac[2] - frac[0])))
        h = max(1, int(g.get("HEIGHT", self.args.portrait_h) * (frac[3] - frac[1])))
        raw = self.script_dir / "_theme_preview_region.png"
        self._run("scrot", str(raw))
        magick = shutil.which("magick") or shutil.which("convert")
        subprocess.run([magick, str(raw), "-crop", f"{w}x{h}+{x0}+{y0}", "+repage", str(dst)],
                       check=True)
        raw.unlink(missing_ok=True)

    def _region_changed(self, before: Path, after: Path) -> bool:
        # Normierter RMSE-Abstand der beiden Ausschnitte. Der Client animiert in
        # diesen Bereichen nichts, solange er auf den Menschen wartet - jede
        # nennenswerte Abweichung heisst also: die Aktion ist angekommen.
        cmp_bin = shutil.which("compare")
        cmd = ([cmp_bin] if cmp_bin else [shutil.which("magick"), "compare"])
        res = subprocess.run(cmd + ["-metric", "RMSE", str(before), str(after), "null:"],
                             capture_output=True, text=True)
        m = re.search(r"\(([0-9.eE+-]+)\)", res.stderr)
        return bool(m) and float(m.group(1)) > 0.01

    def _press_until_change(self, key: str, frac: tuple[float, float, float, float],
                            tries: int, wait_sec: float, desc: str) -> bool:
        # Taste druecken, bis sich der beobachtete Ausschnitt aendert. Ein Druck
        # ins Leere (Austeil-Animation laeuft, Bots sind dran) bleibt folgenlos,
        # ein zweiter kostet dann nichts - eine feste Wartezeit war dagegen mal
        # zu kurz (Aktion verpufft) und mal zu lang (Runde schon weiter).
        # Die Zahl der Versuche ist bewusst eng begrenzt: F3 waehrend der eigenen
        # Aktion ist ein ERHOEHEN, und wer oft genug erhoeht, sitzt am Ende
        # all-in neben dem Tisch - genau das hat die Vorschau schon zerlegt.
        before = self.script_dir / "_theme_preview_before.png"
        after = self.script_dir / "_theme_preview_after.png"
        self._grab_region(frac, before)
        self._run("xdotool", "windowactivate", "--sync", self.win_id, check=False)
        try:
            for _ in range(max(1, tries)):
                self._run("xdotool", "key", "--clearmodifiers", key, check=False)
                time.sleep(wait_sec)
                self._grab_region(frac, after)
                if self._region_changed(before, after):
                    print(f"      {desc}: {key} angekommen")
                    return True
            print(f"      [WARN] {desc}: {key} blieb ohne Wirkung ({tries} Versuche)")
            return False
        finally:
            before.unlink(missing_ok=True)
            after.unlink(missing_ok=True)

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

    def _advance_to_flop(self) -> None:
        # Praeflop liegen keine Community-Cards auf dem Tisch – die Vorschau
        # zeigt dann nur den leeren Filz. Also einmal callen (F2, s. GamePage.qml)
        # und den Bots Zeit lassen, bis der Flop faellt.
        print(f"      Call (F2), bis der Flop liegt (max. {self.args.flop_tries}x) ...")
        self._press_until_change("F2", FRAC_BOARD, self.args.flop_tries,
                                 self.args.flop_sec, "Flop")
        time.sleep(2.0)   # Austeil-Animation der drei Karten

    def _bet_on_flop(self) -> None:
        # Nach dem Flop setzt der Mensch selbst (F3 = Bet/Raise, s. GamePage.qml)
        # und laesst den Bots ein paar Sekunden zum Mitgehen. Ohne diesen Schritt
        # checkt die Runde oft komplett durch und in KEINER Box steht ein Einsatz
        # - die Vorschau zeigte dann nicht, wo der Sitz-Stil den Einsatz ablegt.
        print("      Bet (F3), bis der Einsatz in der Kopfzeile steht "
              f"(max. {self.args.bet_tries}x) ...")
        self._press_until_change("F3", FRAC_HEADER, self.args.bet_tries,
                                 self.args.bet_sec, "Einsatz")
        time.sleep(2.5)   # ein paar Bots ziehen mit, damit mehrere Boxen tragen

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

    def _set_styles_in_config(self) -> None:
        # Setzt die Stil-Keys in der Client-Config (config.xml). Die Keys muessen
        # bereits existieren (Config-Revision aktuell) – ein erster Lauf mit dem
        # Default-Stil upgradet eine alte Config und legt sie an. Kartenstapel und
        # Rueckseite sind eigene Keys: auf dem Tisch liegen offene wie verdeckte
        # Karten, die Vorschau zeigt also beide.
        styles = {"QmlGameTableStyle": self.args.style}
        if self.args.card_deck:
            styles["QmlCardDeckStyle"] = self.args.card_deck
        if self.args.card_back:
            styles["QmlCardBackStyle"] = self.args.card_back
        if self.args.seat_style:
            styles["QmlSeatStyle"] = self.args.seat_style

        cfg = Path(self.args.config).expanduser()
        if not cfg.exists():
            print(f"      [WARN] Config nicht gefunden: {cfg} – Stile nicht gesetzt")
            return
        text = cfg.read_text(encoding="utf-8")
        for key, value in styles.items():
            text, n = re.subn(
                rf'(<{key} value=")[^"]*(")',
                lambda m, v=value: m.group(1) + v + m.group(2),
                text,
            )
            if n == 0:
                print(f"      [WARN] {key}-Key fehlt (alte Config?) – "
                      "erst einen Default-Lauf zum Upgrade ausführen.")
                continue
            print(f"      Config: {key} = {value}")
        cfg.write_text(text, encoding="utf-8")

    def run(self) -> int:
        try:
            if self.args.set_table_style:
                self._set_styles_in_config()
            self._start_services()
            self._wait_for_window()
            self._navigate_to_table()
            if not self.args.preflop:
                self._advance_to_flop()
                if not self.args.no_flop_bet:
                    self._bet_on_flop()
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
    # Vorschau nach dem Flop (Default) – erst dann liegen Community-Cards.
    # Wartezeit je Tastendruck und Zahl der Versuche (s. _press_until_change).
    p.add_argument("--flop-sec", type=float, default=2.0)
    p.add_argument("--flop-tries", type=int, default=8)
    p.add_argument("--preflop", action="store_true",
                   help="Vorschau schon praeflop aufnehmen (ohne Community-Cards)")
    # Eigener Einsatz nach dem Flop, damit in den Spielerboxen Einsaetze stehen.
    p.add_argument("--no-flop-bet", action="store_true",
                   help="nach dem Flop nicht selbst setzen (Runde durchchecken lassen)")
    p.add_argument("--bet-sec", type=float, default=2.5)
    p.add_argument("--bet-tries", type=int, default=3)
    p.add_argument("--binary", default=str(REPO_ROOT / "build/bin/pokerth_qml-client"))
    # Welcher Spieltisch-Stil: bestimmt das Ziel-Verzeichnis
    # (data/gfx/qml/table/<style>/preview*.png).
    p.add_argument("--style", default="default")
    # Stil-Keys vor dem Start in die Client-Config schreiben.
    p.add_argument("--set-table-style", action="store_true")
    # Optional zusaetzlich Kartenstapel/Rueckseite setzen (nur mit
    # --set-table-style wirksam). Ohne Angabe bleibt der konfigurierte Stil.
    p.add_argument("--card-deck", default=None)
    p.add_argument("--card-back", default=None)
    # Sitz-Stil (s. config/SeatStyle.qml): "inset" zeigt den Einsatz im Sockel
    # INNERHALB der Spielerbox, "classic" daneben. Ohne Angabe bleibt der
    # konfigurierte Wert - auf dem Desktop ist das die Vorgabe "inset".
    p.add_argument("--seat-style", default=None, choices=["inset", "classic"])
    p.add_argument("--config", default=str(Path("~/.pokerth/config.xml").expanduser()))
    return p


def main() -> int:
    return ThemePreviewRecorder(build_parser().parse_args()).run()


if __name__ == "__main__":
    raise SystemExit(main())
