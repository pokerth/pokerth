# Build system reference

Supplements [building.md](building.md) and `INSTALL`: **Docker**, **ensure**, mounts, env. Not a full workflow guide. **Implementation invariants** below are the contract; other sections abbreviate the same facts.

## Implementation invariants (do not regress)

Details also in `scripts/functions.sh`.

- **`REPO_BUILD_ROOT`:** `IN_DOCKER=1` + `TARGET_PLATFORM` `windows` \| `android` → `docker/<kind>/build/`. `IN_DOCKER=1` + `linux` (devcontainer `make linux`) → **`build_linux/`** (not `docker/linux/build`).
- **`.manifest.env`:** `$(REPO_BUILD_ROOT)/.manifest.env` (`MANIFEST_NAME` / `MANIFEST_ENV` in Makefile / `functions.sh`). Written by `setup_*.sh`, not by `ensure_docker_deps.py`.
- **Unified image:** `.devcontainer/devcontainer.json` builds `docker/Dockerfile` (`base` + `setup_linux.sh`; `final` + `make setup-toolchains`). `run_devcontainer.py` uses that JSON only.
- **`CACHE_ROOT`:** `~/.pokerth` natively; `/opt/pokerth` in Docker. Toolchains: `${CACHE_ROOT}/{android,windows}`. Qt/vcpkg for `windows` \| `android`: `docker/<kind>/build/{Qt,vcpkg}` (repo tree), not under `CACHE_ROOT`.

**Android:** In `.manifest.env`, `ANDROID_SDK_ROOT` / `ANDROID_NDK_ROOT` must be under `${CACHE_ROOT}/android/...` for the session. Do not reuse one CMake tree across native `build_android/` and Docker `docker/android/build/` without re-setup — paths differ (`~/.pokerth` vs `/opt/pokerth`).

| Flow | `REPO_BUILD_ROOT` | `CACHE_ROOT` (typical) |
| --- | --- | --- |
| Native `make android` (Linux) | `build_android/` | `~/.pokerth` |
| `make android-docker` or devcontainer `make android` | `docker/android/build/` | `/opt/pokerth` (host `~/.pokerth` mounted) |

## Docker workflows (Android / Windows)

**Native vs Docker CMake trees:** `build_<platform>/` on the host vs `docker/<kind>/build/` in container — **do not** point one at the other’s `CMakeCache.txt`. **Toolchains:** one physical tree: host `~/.pokerth` bind-mounts to `/opt/pokerth` in Docker (`IN_DOCKER=1`), so SDK/NDK/MinGW are shared; CMake still uses **different absolute paths** per environment (`/home/.../.pokerth/...` vs `/opt/pokerth/...`) — keep caches separate per workflow.

**Android in Docker — two entry points, same pipeline:**

| How the container starts | Command |
| --- | --- |
| **Host** runs the wrapper | `make android-docker` → `ensure` (if needed), then **`make android`** in the container |
| **Editor** devcontainer | **`make android`** in the integrated terminal (never `make *-docker` inside — Makefile `$(error …)`) |

**Same either way:** one `docker/Dockerfile`, same `.devcontainer/devcontainer.json` / `containerEnv` (`run_devcontainer.py` reads it for host `make *-docker`), **`REPO_BUILD_ROOT=docker/android/build`**, **`docker/android/build/.manifest.env`**.

**Image contents:** `make setup-toolchains` during `docker build` installs SDK/NDK/Gradle (Android) and MinGW (Windows) under `/opt/pokerth/...`. At `docker run`, workspace + `~/.pokerth` → `/opt/pokerth`. Qt/vcpkg arrive at first **`make android`** / **`make windows`** via `setup.sh deps` (or **ensure** before inner `make` on `make *-docker`). No `postCreateCommand`.

**Stale / broken builds:** `make clean`, remove `docker/<kind>/build/.manifest.env`, or delete `docker/android/build` and retry. Missing `build-tools` → stale image or old manifest; rebuild image or refresh manifest.

**Fat APK:** **`FAT_APK=yes`** with **`make android`** / **`make android-docker`** (**`pokerth_client`** only); host env forwarded by **`run_devcontainer.py`**. Full detail: [Android fat APK (reference)](#android-fat-apk-reference).

### Android fat APK (reference)

- **Dispatch:** [`build.sh`](../scripts/build.sh) → [`build_android.sh`](../scripts/build_android.sh): if **`is_yes FAT_APK`** and **`TARGET=pokerth_client`**, **`build_android_fat_apk`** then exit; **`FAT_APK`** + **`pokerth_qml-client`** → error. **`setup_android.sh`** ignores **`FAT_APK`**.
- **Tooling:** One [`docker/Dockerfile`](../docker/Dockerfile); NDK from [`scripts/versions.env`](../scripts/versions.env). **`run_devcontainer.py`** forwards **`FAT_APK`** (like **`CLEAN`**, **`BUILD_TARGET`**).

| | **`pokerth_client`** | **`pokerth_qml-client`** |
| --- | --- | --- |
| **Sources** | `src/gui/qt/android/` | `src/gui/qt6-qml/android/` |
| **Fat APK** | Yes | No (error) |
| **Single-ABI** | Default | **`TARGET`** / **`BUILD_TARGET`** |

**Clean:** `make clean` clears **`build_android/`** and **`docker/android/build/`** (includes **`fat-cmake-*`**, **`fat-universal/`**, **`android-build/`**). **Incremental skip-CMake for fat:** [Planned work](#planned-work). **CI product automation:** [Out of scope](#out-of-scope-for-now).

## Devcontainer terminal

- **`IN_DOCKER=1`** from image; **`IN_DEVCONTAINER=1`** from `devcontainer.json`. **`CACHE_ROOT=/opt/pokerth`**; **`QT_OUTPUT_DIR`** / **`VCPKG_DIR`** under `docker/<kind>/build/` for `make android` / `make windows`. **`make linux`** uses **`build_linux/`** (image runs `setup_linux.sh` at build time).
- **Never** `make *-docker` or `make *-docker-installer` **inside** the container — use `make <kind>` / `ensure_docker_deps.py`.
- Default **`make`** on Linux is still **`linux`** — set **`TARGET_PLATFORM`** / goals explicitly for Android or Windows.

## `setup.sh`, **ensure**, and `.manifest.env`

**Stages:** `all` \| `toolchain` \| `deps`. **`SKIP_SYSTEM_PACKAGES`** gates **apt** only. Docker **final** images run **`toolchain`** only; native **linux** should use **`all`**.

| Stage | Windows | Android |
| --- | --- | --- |
| **Toolchain** | MinGW / NSIS in image | SDK / NDK / Gradle; manifest under `BUILD_DIR` |
| **deps** (at run) | Qt + vcpkg | aqt, vcpkg, protobuf overlays |

**ensure** (Docker only): `make *-docker` → `run_devcontainer.py` → `ensure_docker_deps.py <goal>`. **Ready** when vcpkg (triplet + extras, e.g. `x64-linux`) **and** `qt-cmake` exist under `docker/<kind>/build/Qt`. **Else** `setup.sh deps`, then inner `make`. **ensure** does not write `.manifest.env`; **`setup.sh`** does when the Makefile manifest rule runs.

| | Host `make *-docker` | Devcontainer shell |
| --- | --- | --- |
| **ensure** | Runs before inner `make` | Not on open — run `make android` / `make windows` yourself |
| **`.manifest.env`** | After `setup.sh` via `setup_*`, not by ensure | Same |

## `kind` vs `platform`

| | **`kind`** | **`platform`** |
| --- | --- | --- |
| **Meaning** | Docker pipeline (android / windows) | Build OS (`TARGET_PLATFORM`) |
| **Paths** | `docker/<kind>/build/` when `IN_DOCKER=1` + android/windows | `build_<platform>/` on host |
| **Makefile** | `make <kind>-docker` | `make <platform>`, `make setup-<platform>` |

**linux / macos** have no `kind` — only host trees. **`docker/linux/snap/`**, **`flatpak/`** are packaging recipes, not this devcontainer model.

## Scripts

`setup.sh` → `setup_linux.sh`, `setup_android.sh`, `setup_macos.sh`. **`build.sh`** → `build_linux.sh`, `build_android.sh`, `build_macos.sh`. **`setup_android.sh`:** toolchain → manifest; deps → aqt/vcpkg. Manifest paths: repo `$(REPO_BUILD_ROOT)/.manifest.env` and, when synced, `${CACHE_ROOT}/android/`.

## Environment variables (selected)

| Variable | Role |
| --- | --- |
| `TARGET_PLATFORM` | `setup.sh` dispatch |
| `LAYER` | `toolchain` \| `deps` \| `all` |
| `SKIP_SYSTEM_PACKAGES` | Skip **apt** only; default **no** |
| `VCPKG_DIR`, `VCPKG_ROOT` | Docker: set under `docker/<kind>/build/vcpkg` (`functions.sh`) |
| `BUILD_DIR` | Must match **ensure** / manifest layout in Docker |
| `CACHE_ROOT` | `~/.pokerth` native; `/opt/pokerth` in Docker |
| `QT_OUTPUT_DIR` | `functions.sh`: Docker + android/windows → `docker/<kind>/build/Qt` |
| `MANIFEST_ENV` | Must match **`MANIFEST_NAME`** in Makefile |
| `IN_DOCKER` | Drives `REPO_BUILD_ROOT` (see invariants) |
| `IN_DEVCONTAINER` | Forbids `make *-docker` inside container |
| `SETUP_ALREADY_DONE` | Skip `setup.sh` — requires non-empty `.manifest.env` |
| `DOCKER_FORCE_BUILD` | Force image rebuild (`run_devcontainer.py`) |

**Image vs JSON:** `devcontainer.json` supplies workspace bind + `IN_DEVCONTAINER`; **`IN_DOCKER`** / **`TARGET_PLATFORM`** come from the image. Sync **`docker/Dockerfile`** ↔ **`.devcontainer/devcontainer.json`** (`build.context`, mounts). **`build.options`** / **`runArgs`**: [containers.dev](https://containers.dev).

**vcpkg / CMake staleness:** `invalidate_cmake_vcpkg` and `invalidate_cmake_ndk` in `functions.sh` (used from `build_android.sh`) drop bad `CMakeCache.txt` when roots move. **Android protobuf:** overlay under `$VCPKG_ROOT/vcpkg-overlays/`; ABI changes may need `make clean`. **ensure** may skip `deps` if protobuf configs already exist — delete them or **`CLEAN=yes make android-docker`** if overlays changed. **APK / Gradle:** [building.md](building.md#android-build).

**Image rebuild:** `DOCKER_FORCE_BUILD=1` when `Dockerfile` changes — unchanged tag reuses image. **macOS:** prefer Docker Desktop over Colima for `*-docker`.

## Known issues

- **macOS:** `runArgs --platform linux/amd64` is the **container** arch, not PokerTH `TARGET_PLATFORM`.
- **Colima:** Unreliable for `windows-docker` / `android-docker`.
- **MinGW:** posix model via `setup_linux.sh` / `update-alternatives` when **`TARGET_PLATFORM=windows`**.
- **Bind mounts:** uid mismatch → `chown` on `docker/<kind>/build` or align devcontainer user with host.
- **Stale manifest (host ↔ Docker):** remove `docker/<kind>/build/.manifest.env` or `make clean`. Android in Docker: **`ANDROID_SDK_ROOT`** must stay under `/opt/pokerth/android`.
- **`SETUP_ALREADY_DONE`:** automation must ship a real non-empty `.manifest.env` or the Makefile fails.

---

## Planned work

- **Docker image rebuild detection:** `run_devcontainer.py` skips `docker build` if the tag exists (no `Dockerfile` mtime compare). Use **`DOCKER_FORCE_BUILD=1`** / **`docker rmi`** when the image must change.
- **Android setup:** Optional fail-fast for NDK vs SDK paths (**`build_android.sh`** already guards common cases).
- **Smaller ensure** or Make-only readiness (optional).
- **`functions.sh` / paths:** [Implementation invariants](#implementation-invariants-do-not-regress); full logic in `functions.sh`.
- **Incremental fat APK (future):**
  - **Not implemented** — **`FAT_APK=yes`** always configures + builds both ABIs under **`fat-cmake-<abi>/`**. [Android fat APK (reference)](#android-fat-apk-reference).
  - **Intent:** Skip CMake for an ABI when cache matches Qt/NDK/vcpkg/API; align with **`invalidate_cmake_*`** (**fat** does not call them on **`fat-cmake-*`** yet). Wipe stale **`fat-cmake-<abi>/`** on mismatch. Keep merge / **`androiddeployqt`** / Gradle until designed otherwise; add env toggles later.
- **Legacy `docker/linux/Dockerfiles`:** audit for removal vs missing behavior.

---

## Out of scope for now

- **Docker Compose (experimental / future):** Canonical flows are `.devcontainer/devcontainer.json`, `make *-docker`, and `run_devcontainer.py` — not `docker compose` as a second official path. **[`docker/windows/docker-compose.yml`](../docker/windows/docker-compose.yml)** is a minimal Windows-only example (`target: final`, `IN_DOCKER=1`, workspace mount, no Docker socket). **Strategy to re-implement a universal compose file:** use the **same** `docker/Dockerfile` image as the devcontainer (`target: final` or `base` + manual setup — prefer `final`); bind-mount **repo root** → `/workspaces/pokerth` and **host `~/.pokerth`** → `/opt/pokerth`; set **`IN_DOCKER=1`**; one **service** (or [Compose profile](https://docs.docker.com/compose/how-tos/profiles/)) per target with `TARGET_PLATFORM=windows` \| `android` as needed; do **not** mount a legacy separate vcpkg path onto `/opt/pokerth-windows` — Qt/vcpkg belong under **`docker/<kind>/build/`** per `functions.sh`. Add **`/var/run/docker.sock`** only if you need Docker-in-Docker. **Historical:** older per-kind compose / vcpkg volume layouts were removed with the unified image; restore **behavior** to match `devcontainer.json`, not necessarily old YAML verbatim. **Not CI-tested** — validate `docker compose … run` / `exec` before relying on it.
- **Qt 6.4: backwards compatibility:** revert or keep [QT6.4.patch](../scripts/patches/QT6.4.patch)?
  - `settingsdialog.ui`, `settingsdialogimpl.cpp`: wire sound slider -> label in C++
- **CI** (GHA + cache)
- **Optional minimal Linux setup / thinner unified image** — e.g. skip **linuxdeployqt** download or add a `LAYER`/`deps`-style split for Linux in-image — to shrink image size or build time without changing native `setup_linux.sh` behavior.
- **Legacy docker/linux/Dockerfiles** - investigate if we can remove them or there is functionality we need to restore.
- **Automatic `POKERTH_VERSION` from git describe**
- **vcpkg** cache policy per ABI
- **Windows** long-lived vcpkg staleness.
