# Snap package for PokerTH

This directory holds the `snapcraft.yaml` for the PokerTH snap package (for the
version see `version:` in the `snapcraft.yaml`, currently 2.1.9).

## Overview

- **Base:** core24 (Ubuntu 24.04 LTS)
- **Qt:** 6.9.2 (via aqtinstall, core24 only has 6.4.2)
- **Boost:** 1.88 (from source, core24 only has 1.83)
- **Targets:** `pokerth_qml-client` and `pokerth_client` (Qt Widgets) — the snap
  ships both clients, as the apps `pokerth-qml` and `pokerth`
- **Confinement:** strict

## Build via GitHub Actions

The snap is built by
[`.github/workflows/snap.yml`](../../../.github/workflows/snap.yml).

The workflow is manual only — there is no push trigger: GitHub → Actions →
**Build & Publish Snap** → **Run workflow**. It always checks out the `stable`
branch and takes two inputs:

| Input | Options | Meaning |
| --- | --- | --- |
| `target` | `pokerth-test` (default), `pokerth` | which store snap to publish to; `pokerth-test` is the private test snap |
| `channel` | `stable` (default), `edge` | the Snap Store channel to release to |

The `.snap` artifact can be downloaded under Actions → build run → Artifacts.

## Publishing in the Snap Store

### 1. Create credentials (once)

```bash
sudo snap install snapcraft --classic
snapcraft login
snapcraft export-login --snaps pokerth,pokerth-test --channels stable,edge credentials.txt
```

### 2. Add the GitHub secret

1. GitHub → repo → **Settings** → **Secrets and variables** → **Actions**
2. **New repository secret**
3. Name: `SNAPCRAFT_STORE_CREDENTIALS`
4. Value: the whole content of `credentials.txt`
5. Delete the local file afterwards: `rm credentials.txt`

### 3. Publishing

The workflow run itself uploads to the store, using the `target` and `channel`
selected above.

### Manual upload

```bash
snapcraft upload --release=stable pokerth_2.1.9_amd64.snap
```
