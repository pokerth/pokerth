# Snap-Paket für PokerTH

Dieses Verzeichnis enthält die `snapcraft.yaml` für das PokerTH Snap-Paket (v2.0.6).

## Übersicht

- **Base:** core24 (Ubuntu 24.04 LTS)
- **Qt:** 6.9.2 (via aqtinstall, da core24 nur 6.4.2 hat)
- **Boost:** 1.88 (aus Source, da core24 nur 1.83 hat)
- **Target:** `pokerth_client` (Qt Widgets)
- **Confinement:** strict

## Build via GitHub Actions

Der Snap wird über [`.github/workflows/snap.yml`](../../../.github/workflows/snap.yml) gebaut.

### Automatisch

Bei Push auf `testing` oder `stable` Branch, wenn Dateien in einem dieser Pfade geändert wurden:
- `docker/linux/snap/**`
- `src/**`
- `CMakeLists.txt`

### Manuell

GitHub → Actions → **Build & Publish Snap** → **Run workflow**

Das `.snap`-Artefakt kann unter Actions → Build-Run → Artifacts heruntergeladen werden.

## Veröffentlichen im Snap Store

### 1. Credentials erzeugen (einmalig)

```bash
sudo snap install snapcraft --classic
snapcraft login
snapcraft export-login --snaps pokerth --channels stable,edge credentials.txt
```

### 2. GitHub Secret anlegen

1. GitHub → Repo → **Settings** → **Secrets and variables** → **Actions**
2. **New repository secret**
3. Name: `SNAPCRAFT_STORE_CREDENTIALS`
4. Value: gesamter Inhalt von `credentials.txt`
5. Lokale Datei danach löschen: `rm credentials.txt`

### 3. Automatischer Publish

Push auf den `stable` Branch triggert Build + Upload in den Snap Store.

### Manueller Upload

```bash
snapcraft upload --release=stable pokerth_2.0.6_amd64.snap
```
