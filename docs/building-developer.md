# Build system reference

Supplements [building.md](building.md) (**Pick a workflow**, per-platform steps) and `INSTALL`. **Docker**, **ensure**, mounts, env — not a second copy of the entry-level guide.

## Implementation invariants (do not regress)

Contract for agents and refactors; details elsewhere in this doc and in `scripts/functions.sh`.

- **`REPO_BUILD_ROOT`:** `IN_DOCKER=1` + `TARGET_PLATFORM` `windows` \| `android` → `docker/<kind>/build/`. `IN_DOCKER=1` + `linux` (devcontainer `make linux`) → **`build_linux/`** — same as native, not `docker/linux/build`.
- **`.manifest.env`:** `$(REPO_BUILD_ROOT)/.manifest.env` (basename `MANIFEST_NAME` in `Makefile`, `MANIFEST_ENV` in `functions.sh`). `setup_*.sh` writes it; `ensure_docker_deps.py` does not.
- **Unified image:** `.devcontainer/devcontainer.json` builds `docker/Dockerfile` (`base`: merged apt + `setup_linux.sh`; `final`: `make setup-toolchains`). `run_devcontainer.py` uses that JSON only.
- **`CACHE_ROOT`:** `~/.pokerth` natively; `/opt/pokerth` in Docker (`IN_DOCKER=1`). Toolchains: `${CACHE_ROOT}/{android,windows}`. Qt/vcpkg: `docker/<kind>/build/{Qt,vcpkg}` for `windows` \| `android` only.

**Android:** `ANDROID_SDK_ROOT`, `ANDROID_NDK_ROOT`, etc. in `.manifest.env` must live under `${CACHE_ROOT}/android/...` for the active session. Reusing a manifest or `CMakeCache` across native vs Docker `REPO_BUILD_ROOT` without re-setup can yield wrong NDK/sysroot paths.

| Flow | `REPO_BUILD_ROOT` | `CACHE_ROOT` (typical) |
| --- | --- | --- |
| Native `make android` (Linux host) | `build_android/` | `~/.pokerth` |
| `make android-docker` or devcontainer `make android` | `docker/android/build/` | `/opt/pokerth` in container (host `~/.pokerth` mounted) |

## Host vs In-Docker layout

| Item | Host | Docker/Dev Container |
| --- | --- | --- |
| **Entrypoint** | `make (all\|<platform>)` | `make <kind>-docker` (e.g. `windows-docker`, `android-docker`) |
| **Environment** | `IN_DOCKER` unset | `IN_DOCKER=1`, `IN_DEVCONTAINER=1` |
| **Build tree** | `build_<platform>/` | `windows` / `android`: `docker/<kind>/build/`. `linux` in devcontainer: `build_linux/` (same as native — `docker build` runs `setup_linux.sh` so distro Qt + Boost/protobuf/etc. match the host; `build_linux/.manifest.env` may still come from the host tree after bind-mount). |
| **Setup manifest** | `build_<platform>/.manifest.env` | `windows` / `android`: `docker/<kind>/build/.manifest.env`. `linux`: `build_linux/.manifest.env`. |
| **More** | [building.md](building.md) | With `IN_DOCKER=1`, `functions.sh` sets `VCPKG_DIR` / `QT_OUTPUT_DIR` under `docker/<kind>/build/` only for `windows` and `android`. Android Docker detail: [below](#android-docker-simple-view). |

**Note:** `docker build` cannot mount the repo; the checkout is mounted at `docker run` (one workspace bind; vcpkg/Qt paths follow `functions.sh` when `IN_DOCKER=1`).

### Coexisting native and Docker (Android / Windows cross)

- **Two CMake trees:** native uses `build_<platform>/`; Docker uses `docker/<kind>/build/` — keep both configured; do not point one at the other’s `CMakeCache.txt`.
- **One toolchain directory on disk:** the host’s `~/.pokerth` is bind-mounted to `/opt/pokerth` in the container, so SDK/NDK/MinGW are not duplicated.
- **Different absolute paths in CMake:** native runs use `CACHE_ROOT=~/.pokerth` (paths like `/home/you/.pokerth/...`). Docker runs use `CACHE_ROOT=/opt/pokerth` (paths like `/opt/pokerth/...`). That keeps each environment’s generated CMake files self-consistent; switching workflows does not require reusing the other’s cache.

### Android Docker (simple view)

You are not missing a hidden third workflow — **host `make android-docker` and devcontainer `make android` are the same thing** in terms of trees and env: same `docker/Dockerfile` image, same `.devcontainer/devcontainer.json` `containerEnv` (and `run_devcontainer.py` uses that JSON for host `make *-docker`), same `REPO_BUILD_ROOT=docker/android/build`, same file `docker/android/build/.manifest.env`.

**For all** unified Docker kinds (android + windows), `make setup-toolchains` runs during `docker build`: it invokes `setup.sh toolchain` once per kind (SDK/NDK/Gradle, then MinGW). Same target name — **for all** kinds in the image — not per-platform `make setup-toolchains-android`. After that, `/opt/pokerth/android/...` (SDK) and MinGW under `/opt/pokerth/windows` exist in the image ( `IN_DOCKER=1` → `CACHE_ROOT=/opt/pokerth`). At `docker run`, `.devcontainer/devcontainer.json` bind-mounts the host’s `~/.pokerth` onto `/opt/pokerth`, so the same files appear as `/opt/pokerth/...` in the container and `~/.pokerth/...` on the host without mixing CMake path prefixes between workflows. **Qt and vcpkg are not under `CACHE_ROOT`** — with `IN_DOCKER=1`, `scripts/functions.sh` puts them in `docker/<kind>/build/{Qt,vcpkg}` (repo tree). If `build-tools` is still missing, the image is stale or the manifest on disk predates the unified image — remove `docker/android/build/.manifest.env` or run `make clean` and rebuild the devcontainer.

There is no `postCreateCommand`: Qt/vcpkg (`setup.sh deps`) run when you first `make android` or `make windows` (or when host `make *-docker` runs **ensure** before the inner `make`).

What feels complicated is **implementation**, not the product model:

- **Toolchain cache:** same relative layout `${CACHE_ROOT}/android` and `${CACHE_ROOT}/windows`, with `CACHE_ROOT=~/.pokerth` natively and `CACHE_ROOT=/opt/pokerth` when `IN_DOCKER=1`. The devcontainer mounts **host `~/.pokerth` → `/opt/pokerth`** so one physical tree backs both. Qt/vcpkg for builds stay under `docker/<kind>/build/` (repo tree), not under `CACHE_ROOT`. `setup_android.sh` still has Docker-only branches (e.g. manifest sync) where bind mounts matter. If you previously used `~/.pokerth-android`, move that tree to `~/.pokerth/android` (or re-run setup).
- **Why extra manifest / re-source logic:** the repo file under `docker/android/build/` is bind-mounted from the host; a copy under `${CACHE_ROOT}/android` is a safety net when mounts or stale files disagree. Setup should keep `docker/android/build/.manifest.env` consistent with `${CACHE_ROOT}/android` in Docker.

If anything still fails, `make clean` (or remove `docker/android/build`) and run `make android` / `make android-docker` again so setup regenerates the manifest — same as any other stale-cache issue.

## Dev Container terminal (VS Code / Cursor)

- `IN_DOCKER=1` from `docker/Dockerfile`; `containerEnv` includes `IN_DEVCONTAINER=1` only. `CACHE_ROOT` is `/opt/pokerth` in Docker (host `~/.pokerth` bind-mounted there); toolchain caches are `${CACHE_ROOT}/android` and `${CACHE_ROOT}/windows`. `QT_OUTPUT_DIR` / `VCPKG_DIR` under `docker/<kind>/build/` apply only to `make windows` / `make android` (`functions.sh`). For `make linux` in the devcontainer, use `build_linux/` like on the host. The image runs `scripts/setup_linux.sh` (`USE_AQT=no`) during `docker build` — same package path as native (`LINUX_APT_EXTRA` + system Qt6 apt packages inlined in `setup_linux.sh`). `SKIP_SYSTEM_PACKAGES` is **not** forced in the image (parity with host). `build_linux/.manifest.env` on first open may need `make setup-linux` / `make linux` if your host checkout had no `build_linux/` (workspace bind-mount hides image-layer files under the repo).
- **Never** `make *-docker` / `make *-docker-installer` inside the container (Makefile `$(error …)`). Use `make android`, `make windows`, `…-installer`, or `./scripts/ensure_docker_deps.py <goal>` first.
- **Bare** `make` / `make all` on Linux still default `TARGET_PLATFORM=linux` — use `make android` / `make windows` (or `make linux` on purpose).

## Setup stages (`setup.sh`) and `ensure_docker_deps.py`

**Stages** (one process). **OS packages:** `SKIP_SYSTEM_PACKAGES` gates **apt** only. `ensure_docker_deps.py` does not set `SKIP_SYSTEM_PACKAGES`; other callers may. Unified **Dockerfile** does **not** default `SKIP_SYSTEM_PACKAGES=yes` (native parity).

**Layers:** `all` \| `toolchain` \| `deps`. Docker **final** images run `toolchain` only. Native **linux:** use `all`, not `toolchain`/`deps` alone.

| Stage | Windows | Android |
| --- | --- | --- |
| **Toolchain** | MinGW / NSIS in image; **no** aqt/vcpkg in `final` `RUN` | SDK / NDK / Gradle; `.manifest.env` under `BUILD_DIR` |
| **deps** (at `docker run`) | Qt + vcpkg when `setup.sh deps` (**ensure** may invoke) | aqt, vcpkg, protobuf overlays |

**ensure** — Docker flows only (not native `make setup-android`).

- `make *-docker:` `run_devcontainer.py` → `docker run … ensure_docker_deps.py <goal>`.
- **Ready when:** **vcpkg** ports (triplet + `_KIND_EXTRA_READINESS_TRIPLETS`, e.g. `x64-linux`) **and** `qt-cmake` under `docker/<kind>/build/Qt` (same as `QT_OUTPUT_DIR` from `functions.sh` when `IN_DOCKER=1`).
- **Else:** `setup.sh deps`, then `make` (`IN_DOCKER=1` from the image). `ensure_docker_deps.py` does not set `SKIP_SYSTEM_PACKAGES`; `setup_*.sh` may invoke **apt** again (typically a no-op). `ensure` does not write `.manifest.env`; inner `make` runs the usual manifest prerequisites (`setup.sh` writes `$(REPO_BUILD_ROOT)/.manifest.env`) when needed.

## Ensure vs `.manifest.env` (order)

| Aspect | Host `make *-docker` | Devcontainer shell |
| --- | --- | --- |
| **ensure** | Runs before inner `make` | Not run on container open — run `make android` / `make windows` (or `ensure_docker_deps.py` yourself) |
| `.manifest.env` | Written by `setup_*.sh` when the Makefile manifest rule runs (after `setup.sh`), not by `ensure` | Same |
| **Stale cache** | `deps` even if manifest exists | Same |

Triplets, `SKIP_SYSTEM_PACKAGES`, `containerEnv`: env table below.

## `kind` vs `platform` terminology

| Item | `kind` | `platform` |
| --- | --- | --- |
| **Definition** | Which **Docker/devcontainer** pipeline (image, mounts, **ensure**). | Which **build target** / OS (`TARGET_PLATFORM`). |
| **Values** | `windows` \| `android` | `linux` \| `windows` \| `macos` \| `android` |
| **Paths** | `docker/<kind>/`, `/opt/pokerth/<kind>/` in Docker (host `~/.pokerth/<kind>/` bind-mounted at `/opt/pokerth`) | `build_<platform>/`, `~/.pokerth/<kind>/` on the host |
| **In-Docker tree** | `docker/<kind>/build/` when using this pipeline. | Same dirs when `IN_DOCKER=1` and `TARGET_PLATFORM` is `windows` or `android`. |
| **Makefile** | `make <kind>-docker`; `docker/%/…` (pattern stem `%` is `kind`: `windows` or `android`). | `TARGET_PLATFORM`, `make <platform>`, `make setup-<platform>`; with `IN_DOCKER=1`, `REPO_BUILD_ROOT=docker/<kind>/build` only for `windows` \| `android`; `linux` in the devcontainer uses `build_linux/` (same as native). |
| **Not covered by `kind`** | `make <kind>-docker` and `docker/<kind>/build/` (`REPO_BUILD_ROOT` when `IN_DOCKER=1`) exist only for **windows** and **android** — not for a linux/macos Docker kind. | **linux** and **macos** are **platform**-only (host trees `build_linux/`, `build_macos/`). |

**Packaging:** `docker/linux/snap/` and `docker/linux/flatpak/` are Snap/Flatpak recipe trees for CI — **not** `kind` in the sense above (no `make linux-docker`, no devcontainer `REPO_BUILD_ROOT` there).

## Scripts

- `setup.sh` → `setup_linux.sh` (linux + windows cross), `setup_android.sh`, `setup_macos.sh`.
- `build.sh` → `build_linux.sh`, `build_android.sh`, `build_macos.sh`.
- `setup_android.sh:` **toolchain** → manifest; **deps** → aqt/vcpkg; `build_android.sh` reads `${REPO_ROOT}/${BUILD_DIR}/${MANIFEST_ENV}` and, in Docker, `${CACHE_ROOT}/android`/`${MANIFEST_ENV}` when synced under the toolchain cache (native: `~/.pokerth/android`; Docker: `/opt/pokerth/android` on the same mounted tree).

### Environment variable reference (Docker / ensure)

Stage = `setup.sh` `LAYER` (`toolchain` \| `deps` \| `all`). No `SKIP_QT_INSTALL`.

| Variable | Role | Notes |
| --- | --- | --- |
| `TARGET_PLATFORM` | Dispatch | Required for `setup.sh`. |
| `LAYER` | Stage | **linux** + wrong layer → error. |
| `SKIP_SYSTEM_PACKAGES` | Apt only | Native default **no**. `ensure_docker_deps.py` does not set it. Callers may set `yes` to skip **apt** when packages are already satisfied. Unified **Dockerfile** does not set it in **ENV**. |
| `USE_AQT`, `USE_VCPKG` | Setup | `functions.sh`; windows → both **yes**. |
| `VCPKG_DIR`, `VCPKG_ROOT`, `VCPKG_TRIPLET` | vcpkg | **Docker:** `functions.sh` sets `VCPKG_DIR` / `VCPKG_ROOT` when `IN_DOCKER=1`. **ensure** uses repo paths under `docker/<kind>/build/vcpkg`. |
| `BUILD_DIR` | Manifest path | Must match **ensure** (Docker). |
| `CACHE_ROOT` | Toolchain cache parent | `~/.pokerth` natively; `/opt/pokerth` when `IN_DOCKER=1` (host `~/.pokerth` bind-mounted to `/opt/pokerth`), unless exported earlier. Per-target dirs: `${CACHE_ROOT}/android`, `${CACHE_ROOT}/windows`. |
| `QT_OUTPUT_DIR` | Qt / `qt_cmake_ready` | **Set in `functions.sh`** after `CACHE_ROOT`: `IN_DOCKER=1` → `docker/<kind>/build/Qt`; `TARGET_PLATFORM=windows` and not Docker → `${CACHE_ROOT}/windows/Qt` if unset; else `$HOME/Qt`. `setup_android.sh` may use `${CACHE_ROOT}/android/Qt` where it applies `QT_OUTPUT_DIR:-…`. Override with `QT_OUTPUT_DIR` or `CACHE_ROOT`. |
| `MANIFEST_ENV` | Setup manifest basename | Set once in `functions.sh` (default `.manifest.env`); must match **`MANIFEST_NAME`** in the `Makefile`. Use `"$MANIFEST_ENV"` after sourcing — do not repeat `:-` fallbacks. |
| `IN_DOCKER` | `REPO_BUILD_ROOT` | `Makefile`: `IN_DOCKER=1` + `TARGET_PLATFORM` `windows` \| `android` → `docker/<kind>/build/`; `linux` (e.g. devcontainer **make linux**) → `build_linux/` — same as native. |
| `IN_DEVCONTAINER` | Makefile / editor | `1` from `devcontainer.json` or `REMOTE_CONTAINERS`; forbids `make *-docker` / `make *-docker-installer` (use `make <kind>` / `*-installer` inside the container). |
| `SETUP_ALREADY_DONE` | Makefile | Skip `setup.sh`; **requires** existing non-empty `$(REPO_BUILD_ROOT)/.manifest.env`. |
| `DOCKER_FORCE_BUILD` | Image rebuild | `run_devcontainer.py` |

### Devcontainer vs Dockerfile

- `devcontainer.json:` workspace bind + `IN_DEVCONTAINER=1` only. `IN_DOCKER` and `TARGET_PLATFORM` come from the image; `CACHE_ROOT` from `functions.sh`; `VCPKG_DIR` / `QT_OUTPUT_DIR` → `docker/<kind>/build/` (same as `make *-docker`).
- **Manual sync:** `docker/Dockerfile` ↔ `.devcontainer/devcontainer.json` — `build.context`; `CACHE_ROOT` in `functions.sh`; devcontainer `mounts` bind host `~/.pokerth` → `/opt/pokerth`.
- `build.options` → `docker build`; `runArgs` → `docker run` ([containers.dev](https://containers.dev)).

| Aspect | Windows | Android |
| --- | --- | --- |
| `postCreateCommand` | *(none)* | *(none)* — first `make android` / `make windows` pulls Qt/vcpkg (`deps`) |
| `docker build` (`final`) | `make setup-toolchains` → MinGW under `/opt/pokerth/windows`. No Qt/vcpkg in image. | Same target → SDK/NDK/Gradle + manifest in image. |
| `docker run` mounts | Workspace repo root + host `~/.pokerth` → `/opt/pokerth` | Same; Docker CMake uses `/opt/pokerth/...`; native CMake uses `~/...` — see [Coexisting native and Docker](#coexisting-native-and-docker-android--windows-cross) |
| **Run flow** | **ensure** → `deps`? → `make windows` | **ensure** → `deps`? → `make android` |

### ensure + vcpkg/Qt (short)

- **ensure** `docker_deps_ready`: **vcpkg** (triplet + extras) **and** `qt-cmake` under `docker/<kind>/build/Qt`.
- **Cold cache:** empty `docker/<kind>/build/vcpkg`, then `make *-docker`.
- **Path note:** vcpkg/Qt dirs are `docker/<kind>/build/{vcpkg,Qt}` in the repo (`functions.sh` when `IN_DOCKER=1`).
- **Stale CMake cache:** `invalidate_cmake_vcpkg` and `invalidate_cmake_ndk` (in `scripts/functions.sh`, called from `build_android.sh`; vcpkg only from `build_linux.sh` when `USE_VCPKG=yes`) delete `CMakeCache.txt` and `CMakeFiles/` when vcpkg or NDK roots no longer match, so the same `make` run reconfigures.
- **Android protobuf:** `protobuf:x64-linux` then `protobuf:${VCPKG_TRIPLET}` + overlay under `$VCPKG_ROOT/vcpkg-overlays/` (triplet-scoped to avoid concurrent builds clobbering a shared overlay dir). Setup manifest is not triplet-keyed — change ABI → `make clean` / refresh vcpkg as needed.
- **Overlay changes:** `ensure_docker_deps.py` decides whether to run `setup.sh deps` based on whether protobuf CMake config files already exist in `vcpkg/installed/<triplet>/`. If they exist, overlay-generation changes won’t be reflected until you invalidate protobuf readiness (e.g. remove the protobuf config files for the android triplet or run `CLEAN=yes make android-docker`).
- **APK / Gradle / icon:** [building.md](building.md#android-build) (Android).

**Repeat Android Docker:** `CLEAN=yes make android-docker`; use `DOCKER_FORCE_BUILD=1` (or `docker rmi`) only when you need to **rebuild the image** (not on every `Dockerfile` edit — unchanged tag reuses the image). **macOS:** Docker Desktop over Colima. `localWorkspaceFolder` = **repo root**.

**Targets:** `make windows` / `make android`; `make setup-android` on Linux needs `VCPKG_DIR`.

---

## Known issues

- **macOS Docker:** devcontainer `runArgs --platform linux/amd64` (**Docker** image arch for the devcontainer, not PokerTH `platform`); applies to both kinds.
- **Colima:** unreliable for `windows-docker` / `android-docker` — use Docker Desktop.
- **MinGW:** **posix** thread model — `setup_linux.sh` selects it via `update-alternatives` on Debian/Ubuntu when **`TARGET_PLATFORM=windows`** (native **`make setup-windows`** and Docker **`make setup-toolchains`**).
- **Bind mounts (uid):** Container user (often `vscode` / 1000) may not own host paths under `docker/<kind>/build/` → `Permission denied` on manifest files, **vcpkg**, or **Qt** even when host `ls` looks fine. Mitigations: align uid/gid with the host, avoid **root** writes on mounts, `chown -R "$(id -u):$(id -g)" docker/<kind>/build` on the host.

### Permissions and bind mounts (quick checks)

- **`Permission denied` under `docker/windows/build` or `docker/android/build`:** On the host, `chown -R "$(id -u):$(id -g)" docker/<kind>/build` (or fix uid/gid in `devcontainer.json` / image to match your user).
- **Stale manifest after switching host vs Docker:** Remove `docker/<kind>/build/.manifest.env` or run `make clean`, then `make <kind>` again. For Android in Docker, `ANDROID_SDK_ROOT` must be under `/opt/pokerth/android` — `build_android.sh` errors if a host-only path leaked into `.manifest.env`.

### CI and `SETUP_ALREADY_DONE`

- If automation sets **`SETUP_ALREADY_DONE`**, the Makefile requires a **non-empty** **`$(REPO_BUILD_ROOT)/.manifest.env`** (populate from a real setup run or commit a generated file intentionally). Empty files fail the manifest rule.

---

## Planned work

- **fat APK** / `Dockerfile.universal`
- **Find a way to more reliably detect if a Docker image needs rebuilding:** `run_devcontainer.py:` skips `docker build` if the image tag already exists (no comparison to `Dockerfile` mtime). Editing `docker/Dockerfile` does **not** by itself run `docker build` on the next `make *-docker`. Rebuild when you need a fresh image: `DOCKER_FORCE_BUILD=1`, `--force-build`, or `docker rmi` the tag. Forwards `CLEAN`, `BUILD_TARGET`. `.dockerignore` trims context.
- **Android manifest / SDK paths:** Repo manifest `${REPO_ROOT}/<build-dir>/${MANIFEST_ENV}` (`REPO_BUILD_ROOT` / `build_android` in **`build_android.sh`**; **`BUILD_DIR`** in **`setup_android.sh`**); Docker cache copy `${CACHE_ROOT}/${TARGET_PLATFORM}/${MANIFEST_ENV}`. Optional **fail-fast** for `ANDROID_NDK_ROOT` vs `ANDROID_SDK_ROOT` in setup ( **`build_android.sh`** already checks the NDK toolchain file and errors in Docker if `ANDROID_SDK_ROOT` is not under `${CACHE_ROOT}/android`).
- **Manifest vs ensure:** Ensure vs `.manifest.env` above; optional: **Make**-only readiness or smaller **ensure**.
- `functions.sh` / path env (native vs devcontainer): [Implementation invariants](#implementation-invariants-do-not-regress) above; full behavior in this doc and `functions.sh`.

---

## Out of scope for now

- **Docker Compose (experimental / future):** Canonical flows are `.devcontainer/devcontainer.json`, `make *-docker`, and `run_devcontainer.py` — not `docker compose` as a second official path. **`docker/windows/docker-compose.yml`** is a minimal Windows-only example (`target: final`, `IN_DOCKER=1`, workspace mount, no Docker socket). **Strategy to re-implement a universal compose file:** use the **same** `docker/Dockerfile` image as the devcontainer (`target: final` or `base` + manual setup — prefer `final`); bind-mount **repo root** → `/workspaces/pokerth` and **host `~/.pokerth`** → `/opt/pokerth`; set **`IN_DOCKER=1`**; one **service** (or [Compose profile](https://docs.docker.com/compose/how-tos/profiles/)) per target with `TARGET_PLATFORM=windows` \| `android` as needed; do **not** mount a legacy separate vcpkg path onto `/opt/pokerth-windows` — Qt/vcpkg belong under **`docker/<kind>/build/`** per `functions.sh`. Add **`/var/run/docker.sock`** only if you need Docker-in-Docker. **Historical:** older per-kind compose / vcpkg volume layouts were removed with the unified image; restore **behavior** to match `devcontainer.json`, not necessarily old YAML verbatim. **Not CI-tested** — validate `docker compose … run` / `exec` before relying on it.
- **Qt 6.4: backwards compatibility:** revert or keep [QT6.4.patch](../scripts/patches/QT6.4.patch)?
  - `settingsdialog.ui`, `settingsdialogimpl.cpp`: wire sound slider -> label in C++
- **CI** (GHA + cache)
- **Optional minimal Linux setup in the unified Docker image** — e.g. skip **linuxdeployqt** download or add a `LAYER`/`deps`-style split for Linux in-image — to shrink image size or build time without changing native `setup_linux.sh` behavior.
- **Legacy docker/linux/Dockerfiles** - investigate if we can remove them or there is functionality we need to restore.
- **Automatic `POKERTH_VERSION` from git describe**
- **vcpkg** cache per ABI
- **Windows** long-lived vcpkg staleness.
