# Android (Docker / devcontainer)

Quick entry for `make android-docker` and **Reopen in Container**. **Canonical detail:** [building-developer.md](../../docs/building-developer.md) (ensure, `CACHE_ROOT`, `.manifest.env`, `REPO_BUILD_ROOT`). **Workflow overview:** [building.md](../../docs/building.md).

## Prerequisites

- Docker
- VS Code Dev Containers (optional): `ms-vscode-remote.remote-containers`

## Quick start

From **repo root**:

- **Host:** `make android-docker` — `scripts/run_devcontainer.py` reads `.devcontainer/devcontainer.json` (same image as **Reopen in Container**).
- **Editor:** Open the repo root → **Dev Containers: Reopen in Container** → run `make android` in the integrated terminal.

The image includes **SDK/NDK/Gradle** and **MinGW** from `docker build` (`make setup-toolchains`). **Qt (aqt)** and **vcpkg** install on first `make android` when `ensure_docker_deps.py` runs `setup.sh deps`. `ensure` does not write `.manifest.env`; `setup_android.sh` does when the Makefile manifest rule runs.

## Outputs

- **Docker / devcontainer:** unsigned APK under  
  `docker/android/build/android-build/build/outputs/apk/release/`  
  (exact name may vary, e.g. `*-release-unsigned.apk`).
- **Native Linux (no Docker):** same layout under `build_android/android-build/...` with `REPO_BUILD_ROOT=build_android`.

**Launcher icon:** `res/drawable/ic_launcher.png` is a **symlink** to `data/gfx/gui/misc/windowicon_transparent.png`. On Windows clones, enable Git symlink support or build in Docker/Linux. `CLEAN=yes make android-docker` wipes `docker/android/build/android-build` before packaging when you need a clean Gradle tree.

**Other ABIs:** `make android ANDROID_BUILD_ARGS="--arch x86_64"` — see `scripts/build_android.sh --help` and **building-developer.md** for `VCPKG_TRIPLET`.

**Host Linux (native tree):** e.g. `export VCPKG_DIR=build_android/vcpkg`, then `make setup-android`, then `make android`.

## Sign the APK

Generate keystore (first time only):

```bash
keytool -genkey -v -keystore my.keystore -keyalg RSA -keysize 2048 -validity 10000 -alias app
```

Sign (Docker path; use `build_android/...` for a host tree):

```bash
apksigner sign --ks my.keystore --ks-key-alias app \
  docker/android/build/android-build/build/outputs/apk/release/android-build-release-unsigned.apk
```
