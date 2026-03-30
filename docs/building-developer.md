# Build system reference

Supplements [building.md](building.md) (**Pick a workflow**, per-platform steps) and `INSTALL`. **Docker**, **ensure**, mounts, env — not a second copy of the entry-level guide.

## Host vs In-Docker layout

| Item | Host | Docker/Dev Container |
| --- | --- | --- |
| **Entrypoint** | `make (all\|<platform>)` | `make <kind>-docker` (e.g. `windows-docker`, `android-docker`) |
| **Environment** | `IN_DOCKER` unset | `IN_DOCKER=1`, `IN_DEVCONTAINER=1` |
| **Build tree** | `build_<platform>/` | `docker/<kind>/build/` |
| **Stamp** | `build_<platform>/.stamp_setup` | `docker/<kind>/build/.stamp_setup` |
| **More** | [building.md](building.md) | Mounts vcpkg/Qt → `/opt/pokerth-<kind>/…` |

**Note:** `docker build` cannot mount directories on the host, all bind mounts of cache directories are done at `docker run`.

## Dev Container terminal (VS Code / Cursor)

- `ENV IN_DOCKER=1` in `docker/<kind>/Dockerfile`; `containerEnv` `IN_DEVCONTAINER=1` → `REPO_BUILD_ROOT=docker/$(TARGET_PLATFORM)/build` (here `TARGET_PLATFORM` is `windows` or `android` — i.e. `kind`; same path as `docker/<kind>/build`).
- **Never** `make *-docker` / `make *-docker-installer` inside the container (Makefile `$(error …)`). Use `make android`, `make windows`, `…-installer`, or `./scripts/ensure_docker_deps.py <goal>` first.
- **Bare** `make` / `make all` on Linux still default `TARGET_PLATFORM=linux` — use `make android` / `make windows` (or `make linux` on purpose).

## Setup stages (`setup.sh`) and `ensure_docker_deps.py`

**Stages** (one process). **OS packages:** `SKIP_SYSTEM_PACKAGES` gates **apt** only — **Dockerfile** **base** vs native `make setup-*`.

**Layers:** `all` \| `toolchain` \| `deps`. Docker **final** images run `toolchain` only. Native **linux:** use `all`, not `toolchain`/`deps` alone.

| Stage | Windows | Android |
| --- | --- | --- |
| **Toolchain** | MinGW / NSIS in image; **no** aqt/vcpkg in `final` `RUN` | SDK / NDK / Gradle; `.manifest.env` under `BUILD_DIR` |
| **deps** (at `docker run`) | Qt + vcpkg when `setup.sh deps` (**ensure** may invoke) | aqt, vcpkg, protobuf overlays |

**ensure** — Docker flows only (not native `make setup-android`).

- `make *-docker:` `run_devcontainer.py` → `docker run … ensure_docker_deps.py <goal>`.
- **Ready when:** **vcpkg** ports (triplet + `_KIND_EXTRA_READINESS_TRIPLETS`, e.g. `x64-linux`) **and** `qt-cmake` under `QT_OUTPUT_DIR` / `${ROOT}/Qt`.
- **Else:** `setup.sh deps`, `SKIP_SYSTEM_PACKAGES=yes`, touch `.stamp_setup`, `make` with `IN_DOCKER=1`.

## Ensure vs `.stamp_setup` (order)

| Aspect | Host `make *-docker` | Devcontainer shell |
| --- | --- | --- |
| **ensure** | Runs before inner `make` | `postCreate` (Windows) or manual `ensure` only |
| `.stamp_setup` | **ensure** may `deps` + touch `docker/.../` | `make` + stamp rules |
| **Stale cache** | `deps` even if stamp exists | Same |

Triplets, `SKIP_SYSTEM_PACKAGES`, `containerEnv`: env table below.

## `kind` vs `platform` terminology

| Item | `kind` | `platform` |
| --- | --- | --- |
| **Definition** | Which **Docker/devcontainer** pipeline (image, mounts, **ensure**). | Which **build target** / OS (`TARGET_PLATFORM`). |
| **Values** | `windows` \| `android` | `linux` \| `windows` \| `macos` \| `android` |
| **Paths** | `docker/<kind>/`, `.devcontainer/<kind>/`, `/opt/pokerth-<kind>/` | `build_<platform>/` |
| **In-Docker tree** | `docker/<kind>/build/` when using this pipeline. | Same dirs when `IN_DOCKER=1` and `TARGET_PLATFORM` is `windows` or `android`. |
| **Makefile** | `make <kind>-docker`; `docker/%/…` (pattern stem `%` is `kind`: `windows` or `android`). | `TARGET_PLATFORM`, `make <platform>`, `make setup-<platform>`; with `IN_DOCKER=1`, `REPO_BUILD_ROOT=docker/$(TARGET_PLATFORM)/build` (then `TARGET_PLATFORM` is always a `kind`). |
| **Not covered by `kind`** | `make <kind>-docker` and `docker/<kind>/build/` (`REPO_BUILD_ROOT` when `IN_DOCKER=1`) exist only for **windows** and **android** — not for a linux/macos Docker kind. | **linux** and **macos** are **platform**-only (host trees `build_linux/`, `build_macos/`). |

**Packaging:** `docker/linux/snap/` and `docker/linux/flatpak/` are Snap/Flatpak recipe trees for CI — **not** `kind` in the sense above (no `make linux-docker`, no devcontainer `REPO_BUILD_ROOT` there).

## Scripts

- `setup.sh` → `setup_linux.sh` (linux + windows cross), `setup_android.sh`, `setup_macos.sh`.
- `build.sh` → `build_linux.sh`, `build_android.sh`, `build_macos.sh`.
- `setup_android.sh:` **toolchain** → manifest; **deps** → aqt/vcpkg; `build_android.sh` reads `${REPO_ROOT}/${BUILD_DIR}/${MANIFEST_ENV}` or `${ROOT}/${MANIFEST_ENV}` (Docker).

### Environment variable reference (Docker / ensure)

Stage = `setup.sh` `LAYER` (`toolchain` \| `deps` \| `all`). No `SKIP_QT_INSTALL`.

| Variable | Role | Notes |
| --- | --- | --- |
| `TARGET_PLATFORM` | Dispatch | Required for `setup.sh`. |
| `LAYER` | Stage | **linux** + wrong layer → error. |
| `SKIP_SYSTEM_PACKAGES` | Apt only | `Dockerfile` `ENV` when base apt ran. |
| `USE_AQT`, `USE_VCPKG` | Setup | `functions.sh`; windows → both **yes**. |
| `VCPKG_DIR`, `VCPKG_ROOT`, `VCPKG_TRIPLET` | vcpkg | **ensure:** `VCPKG_ROOT` or `$ROOT/vcpkg`. |
| `BUILD_DIR` | Stamps, manifest | Must match **ensure** (Docker). |
| `ROOT` | Bind root | `/opt/pokerth-<kind>` |
| `QT_OUTPUT_DIR` | Qt / `qt_cmake_ready` | Optional. **Windows:** **setup_linux.sh** sets `${ROOT}/Qt` from **ROOT** when unset. **Android:** **setup_android.sh** uses `${ANDROID_CACHE_ROOT}/Qt` when unset. **devcontainer.json** may set **QT_OUTPUT_DIR** for the IDE. |
| `MANIFEST_ENV` | Android manifest | Default `.manifest.env`. |
| `IN_DOCKER` | `REPO_BUILD_ROOT` | `docker/$(TARGET_PLATFORM)/build` |
| `IN_DEVCONTAINER` | Makefile / editor | `1` from `devcontainer.json` or `REMOTE_CONTAINERS`; forbids `make *-docker` / `make *-docker-installer` (use `make <kind>` / `*-installer` inside the container). |
| `SETUP_ALREADY_DONE` | Makefile | Touch stamp only. |
| `DOCKER_FORCE_BUILD` | Image rebuild | `run_devcontainer.py` |

### Devcontainer vs Dockerfile

- `devcontainer.json:` binds + **run** env. `docker build:` `final` = `setup.sh toolchain` only — no host `docker/<kind>/build` mounts (`downloads/` at **run** use mounts below).
- **Manual sync:** `Dockerfile` ↔ `.devcontainer/.../devcontainer.json` — `build.context`, mount paths, `/opt/pokerth-*` `ENV`, `containerEnv`.
- `build.options` → `docker build`; `runArgs` → `docker run` ([containers.dev](https://containers.dev)).

| Aspect | Windows | Android |
| --- | --- | --- |
| `postCreateCommand` | `ensure_docker_deps.py windows` → `make windows` | *(none)* |
| `docker build` (`final`) | `setup.sh toolchain` → MinGW `/opt/pokerth-windows`. No Qt/vcpkg in image. | `setup.sh toolchain` → SDK/NDK/Gradle + manifest in image. |
| `docker run` mounts | `docker/windows/build/{vcpkg,Qt}` → `/opt/pokerth-windows/{vcpkg,Qt}` | `docker/android/build/{vcpkg,Qt}` → `/opt/pokerth-android/{vcpkg,Qt}` |
| **Run flow** | **ensure** → `deps`? → `make windows` | **ensure** → `deps`? → `make android` |

### ensure + vcpkg/Qt (short)

- **ensure** `docker_deps_ready`: **vcpkg** (triplet + extras) **and** `qt-cmake` under `QT_OUTPUT_DIR` / `${ROOT}/Qt`.
- **Cold cache:** empty `docker/<kind>/build/vcpkg`, then `make *-docker`.
- **Path note:** container vcpkg is `/opt/pokerth-<kind>/vcpkg`, not `…/build/vcpkg`.
- **Android protobuf:** `protobuf:x64-linux` then `protobuf:${VCPKG_TRIPLET}` + overlay under `$VCPKG_ROOT/vcpkg-overlays/` (triplet-scoped to avoid concurrent builds clobbering a shared overlay dir). `.stamp_setup` not triplet-keyed — change ABI → clean stamp / vcpkg.
- **Overlay changes:** `ensure_docker_deps.py` decides whether to run `setup.sh deps` based on whether protobuf CMake config files already exist in `vcpkg/installed/<triplet>/`. If they exist, overlay-generation changes won’t be reflected until you invalidate protobuf readiness (e.g. remove the protobuf config files for the android triplet or run `CLEAN=yes make android-docker`).
- **APK / Gradle / icon:** [building.md](building.md#android-build) (Android).

**Repeat Android Docker:** `CLEAN=yes make android-docker`; use `DOCKER_FORCE_BUILD=1` (or `docker rmi`) only when you need to **rebuild the image** (not on every `Dockerfile` edit — unchanged tag reuses the image). **macOS:** Docker Desktop over Colima. `localWorkspaceFolder` = **repo root**.

**Targets:** `make windows` / `make android`; `make setup-android` on Linux needs `VCPKG_DIR`.

---

## Known issues

- **macOS Docker:** devcontainer `runArgs --platform linux/amd64` (**Docker** image arch for the devcontainer, not PokerTH **`platform`**); applies to both kinds.
- **Colima:** unreliable for `windows-docker` / `android-docker` — use Docker Desktop.
- **MinGW:** **posix** thread model in Dockerfile.
- **Bind mounts (uid):** Container user (often `vscode` / 1000) may not own host paths under `docker/<kind>/build/` → `Permission denied` on stamps, **vcpkg**, or **Qt** even when host `ls` looks fine. Mitigations: align uid/gid with the host, avoid **root** writes on mounts, `chown -R "$(id -u):$(id -g)" docker/<kind>/build` on the host. **`touch_stamp_file`** in **ensure** only helps stamp files, not an unwritable cache dir.

---

## Planned work

- **Find a way to more reliably detect if a Docker image needs rebuilding:** `run_devcontainer.py:` skips `docker build` if the image tag already exists (no comparison to `Dockerfile` mtime). Editing `docker/<kind>/Dockerfile` does **not** by itself run `docker build` on the next `make *-docker`. Rebuild when you need a fresh image: `DOCKER_FORCE_BUILD=1`, `--force-build`, or `docker rmi` the tag. Forwards `CLEAN`, `BUILD_TARGET`. `.dockerignore` trims context.
- **Merge devcontainers:** Replace separate `.devcontainer/windows/` and `.devcontainer/android/` with **one** devcontainer (one image or one shared definition) that can run **`make linux`**, **`make windows`**, and **`make android`** as needed—same repo root, coherent mounts under `docker/<kind>/build/`, and alignment with **`run_devcontainer.py`** / **`make *-docker`** so host and editor flows stay consistent.
- **Android manifest:** unify `${REPO_ROOT}/${BUILD_DIR}/${MANIFEST_ENV}` and discovery (no separate doc in repo yet).
- **Stamps vs ensure:** Ensure vs `.stamp_setup` above; optional: **Make**-only readiness or smaller **ensure**.
- **Permissions doc:** Decide whether the **Known issues** note above plus **build**/**run** split is enough, or add a short troubleshooting subsection elsewhere.
- **`functions.sh` / path env (native vs devcontainer):** Tracked under the **merge-devcontainer** effort—see [merge-devcontainers.md](plans/merge-devcontainers.md) (**Scripts, `functions.sh`, and path env**): one path model for `REPO_BUILD_ROOT` / `ROOT` / Qt / vcpkg, lower LoC, and concrete near-term dedupes (e.g. Windows Qt kit path, `VCPKG_PORTS` data, `ensure` vs bash readiness).

---

## Out of scope for now

- **Automatic `POKERTH_VERSION` from git describe**
- **fat APK** / `Dockerfile.universal`
- **CI** (GHA + cache)
- **Qt 6.4** UI revert?
- **vcpkg** cache per ABI
- **Windows** long-lived vcpkg staleness.
