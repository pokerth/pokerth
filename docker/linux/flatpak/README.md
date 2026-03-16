# Flatpak-Paket für PokerTH

Dieses Verzeichnis enthält das Flatpak-Manifest für PokerTH (v2.0.6).

## Übersicht

- **App-ID:** `net.pokerth.PokerTH`
- **Runtime:** org.kde.Platform 6.9 (liefert Qt 6.9.x, PulseAudio, GStreamer)
- **SDK:** org.kde.Sdk 6.9
- **Boost:** 1.88 (aus Source)
- **Protobuf:** 3.21.12 (aus Source)
- **WebSocket++:** 0.8.2 (aus Source)
- **Nimbus Sans L Font:** aus altem gsfonts-Paket (Ubuntu)
- **Target:** `pokerth_client` (Qt Widgets)

## Build via GitHub Actions

Der Flatpak wird über [`.github/workflows/flatpak.yml`](../../../.github/workflows/flatpak.yml) gebaut.

### Manuell

GitHub → Actions → **Build & Publish Flatpak** → **Run workflow**

Der Build checkt immer den `stable`-Branch aus.

Das `.flatpak`-Bundle kann unter Actions → Build-Run → Artifacts heruntergeladen werden.

### Lokal installieren

```bash
# Bundle aus den GitHub Actions Artifacts herunterladen, dann:
flatpak install --user pokerth.flatpak
flatpak run net.pokerth.PokerTH
```

## Lokal installieren

```bash
# Bundle herunterladen, dann:
flatpak install --user pokerth.flatpak
flatpak run net.pokerth.PokerTH
```

## Flathub-Veröffentlichung

Für die Veröffentlichung auf Flathub siehe:
https://docs.flathub.org/docs/for-app-authors/submission
