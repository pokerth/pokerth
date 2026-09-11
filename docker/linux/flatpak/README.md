# Flatpak package for PokerTH

This directory holds the Flatpak manifest for PokerTH (currently 2.1.8).

## Overview

- **App ID:** `net.pokerth.PokerTH`
- **Runtime:** org.kde.Platform 6.9 (provides Qt 6.9.x, PulseAudio, GStreamer)
- **SDK:** org.kde.Sdk 6.9
- **Boost:** 1.88 (from source)
- **Protobuf:** 3.21.12 (from source)
- **WebSocket++:** 0.8.2 (from source)
- **Nimbus Sans L font:** from the old gsfonts package (Ubuntu)
- **Target:** `pokerth_client` (Qt Widgets)

## Build via GitHub Actions

The Flatpak is built by
[`.github/workflows/flatpak.yml`](../../../.github/workflows/flatpak.yml).

The workflow is manual only: GitHub → Actions → **Build & Publish Flatpak** →
**Run workflow**. It always checks out the `stable` branch.

The `.flatpak` bundle can be downloaded under Actions → build run → Artifacts.

## Install locally

```bash
# download the bundle from the GitHub Actions artifacts, then:
flatpak install --user pokerth.flatpak
flatpak run net.pokerth.PokerTH
```

## Publishing on Flathub

See https://docs.flathub.org/docs/for-app-authors/submission
