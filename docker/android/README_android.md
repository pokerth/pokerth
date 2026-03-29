## Prerequisites

- Docker
- VS Code with Dev Containers extension (ms-vscode-remote.remote-containers)

## Build Instructions

**Command line (recommended):** From **repo root**, `make android-docker`. `scripts/run_devcontainer.py` reads `.devcontainer/android/devcontainer.json` (same as VS Code/Cursor): `build.options` (e.g. `--platform=linux/amd64` for the **Docker image** arch, not PokerTH `TARGET_PLATFORM`) is passed to `docker build`; `runArgs` to `docker run`.

Flow: container starts → `ensure_docker_deps.py android` — if **vcpkg** or **Qt** (`qt-cmake` under the mounted Qt tree) checks fail, it runs `setup.sh deps` (`TARGET_PLATFORM=android` → `setup_android.sh`; ports into `docker/android/build/vcpkg` via mount), sets `BUILD_DIR` to `docker/android/build` so **`.manifest.env`** and **`.stamp_setup`** stay aligned with `REPO_BUILD_ROOT` — then `make android` → `build.sh` → `build_android.sh`.

**APK build:** `build_android.sh` invokes `androiddeployqt` with `--release` (Qt runs `buildAndroidProject()` and Gradle `assembleRelease`). `res/drawable/ic_launcher.png` is a **symlink** to `data/gfx/gui/misc/windowicon_transparent.png` under each client’s `android/res/` tree (same icon, no duplicate PNGs). On **Windows** clones, enable Git symlink support if links show as plain files (`git config core.symlinks true`, Developer Mode, or run Android builds in Docker/Linux). `CLEAN=yes make android-docker` removes `docker/android/build/android-build` before packaging when you need a clean Gradle/Android tree.

**Mounts:** Repo → `/workspaces/pokerth`. Host `docker/android/build/vcpkg` → `/opt/pokerth-android/vcpkg` and `docker/android/build/Qt` → `/opt/pokerth-android/Qt` (SDK/NDK/Gradle stay in the image under `/opt/pokerth-android`, not hidden by the workspace mount).

**Finding Qt on disk:** `docker/android/build/Qt` (e.g. `…/gcc_64`) is next to `docker/android/build/vcpkg`. That tree is listed in `.gitignore` (`docker/**/build/`), so Git (and some IDEs) will not show it as untracked — use `ls docker/android/build` in a shell to confirm.

**Image:** The Dockerfile `final` stage runs `setup.sh toolchain` only (SDK/NDK/Gradle + `.manifest.env`). Qt (`aqt`) and vcpkg ports run on `docker run` (**ensure** invokes `setup.sh deps` when protobuf/vcpkg is not ready). `${ROOT}/.manifest.env` on `/opt/pokerth-android` survives the repo bind mount (see `setup_android.sh` `_sync_manifest_to_root`).

**APK (Docker):** `docker/android/build/android-build/build/outputs/apk/release/` (unsigned). **Host** `make android`: `build_android/android-build/...` instead.

Other ABIs: `make android ANDROID_BUILD_ARGS="--arch x86_64"` (`scripts/build_android.sh --help`). Align `VCPKG_TRIPLET` with the ABI when pre-seeding vcpkg (see [building-developer.md](../../docs/building-developer.md)).

**Host Linux:** `export VCPKG_DIR=build_android/vcpkg` (example) then `make setup-android` — full SDK/NDK/Gradle/Qt + vcpkg (same script path as Docker ensure), then `make android`.

**Not used:** `install_vcpkg_android.sh` (if present in old trees) — vcpkg is handled by `setup_android.sh` / `make setup-android` / `setup.sh` (`TARGET_PLATFORM=android`).

**VS Code Dev Container:** Open the **repository root** → **Dev Containers: Rebuild and Reopen in Container** (pick `.devcontainer/android/devcontainer.json`). Repo at `/workspaces/pokerth`; binds `vcpkg` and `Qt` as above. With toolchain in the image, `make android` is usually enough once vcpkg is populated; use `make setup-android` if you need to refresh ports (`VCPKG_DIR` must point at your vcpkg root). `make android-docker` from the host uses the same `devcontainer.json`.

```bash
make android
```

Unsigned APK (typical path; exact name may vary):

```text
docker/android/build/android-build/build/outputs/apk/release/android-build-release-unsigned.apk
```

## Sign the APK

Generate keystore (first time only):

```bash
keytool -genkey -v -keystore my.keystore -keyalg RSA -keysize 2048 -validity 10000 -alias app
```

Sign the APK (Docker output path; use `build_android/...` for a host tree):

```bash
apksigner sign --ks my.keystore --ks-key-alias app \
  docker/android/build/android-build/build/outputs/apk/release/android-build-release-unsigned.apk
```
