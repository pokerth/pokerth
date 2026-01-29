# Building: what changed, how it works, what’s next

Developer-oriented. **User-facing build instructions:** **docs/building.md** and **INSTALL**. Targets, flow, platform details, script roles, and troubleshooting are there; this doc covers refactor history, dev-only details, and future work.

---

## 1. What changed (relative to last commit / pre-refactor)

**Entry point and scripts**

- **Makefile** at repo root is the only build/setup entry (no root-level `build_linux.sh` / `setup_linux.sh` / `build_windows.sh` / `setup_windows.sh`).
- All build/setup logic lives under **scripts/** (`build.sh`, `setup.sh`, `build_macos.sh`, `setup_macos.sh`, `clean_build.sh`, `functions.sh`). Linux and Windows share one script each; **TARGET_PLATFORM** selects the target.
- **REPO_ROOT** is used for all repo-relative paths; set in `scripts/functions.sh`. **SCRIPT_DIR** was removed.

**Setup and installers**

- **make setup-linux** (Linux target) installs **linuxdeployqt** to `~/bin` if missing. **scripts/build.sh** adds `~/bin` to PATH in `create_linux_appimage()` when linuxdeployqt is not in PATH so **make linux-installer** works without the user editing PATH.
- Ubuntu 22.04 vs 24.04, USE_AQT=yes, linuxdeployqt “host too new”, and “After upgrading 22.04 → 24.04” are documented in **building.md**.

**Messages and docs**

- Script completion and error messages now say **make &lt;target&gt;** or **scripts/&lt;script&gt;** instead of `./build_macos.sh` at root. **scripts/functions.sh** default setup_script text updated accordingly.
- **INSTALL** and **building.md** point to the Makefile as the main entry.

**Miscellanous fixes**

- **Settings dialog**: sound volume slider→label connection moved from .ui to **settingsdialogimpl.cpp** (lambda) to avoid Qt 6.4 uic overload ambiguity on `QLabel::setNum`.

---

## 2. How things work

**Entry and flow**

- **building.md** has the full list of Makefile targets, platform flow (setup → build → optional installer), and script roles. Scripts under **scripts/** are invoked with the right env (e.g. `TARGET_PLATFORM=windows`, `CREATE_INSTALLER=yes`). Pass-through env: `CLEAN=yes`, `BUILD_TARGET=…`, etc.

**scripts/ (minimal)**

- **functions.sh** — Shared env (QT_VERSION, VCPKG_DIR, REPO_ROOT, etc.) and helpers; sourced by other scripts.
- **setup.sh** — Linux host: deps for **linux** or **windows** (TARGET_PLATFORM). Base packages, Qt (system or aqtinstall), optional vcpkg; Linux target also installs linuxdeployqt to ~/bin if missing.
- **build.sh** — Linux host: build for **linux** or **windows** (TARGET_PLATFORM). Configure, build, deploy dir, optional installer (AppImage / NSIS), summary.
- **setup_macos.sh** / **build_macos.sh** — macOS host: Homebrew, pipx/aqtinstall, vcpkg, Qt; build and optional DMG.
- **clean_build.sh** — Removes build_linux/, build_windows/, build_macos/.

**Important env (for scripts)**

- **REPO_ROOT** — Repo root (set in functions.sh). All paths to data/, docker/windows/, CMakeLists.txt, build_* use REPO_ROOT.
- **TARGET_PLATFORM** — `linux` or `windows` for build.sh / setup.sh (set by Makefile for windows).
- **USE_AQT** — Use aqtinstall for Qt (required for Windows; optional for Linux).
- **VCPKG_DIR**, **QT_OUTPUT_DIR**, **QT_VERSION** — See scripts/functions.sh.
- **CLEAN**, **CREATE_INSTALLER**, **BUILD_TARGET** — Pass-through for build scripts.

**Where to look**

- **User build:** docs/building.md, INSTALL.
- **This doc:** §1 (what changed), §3 (future).
- **Implementation:** scripts/*.sh (functions.sh for VCPKG_PORTS / QT_MODULES).
- **Docker/CI:** docker/windows/, docker/android/; currently separate from scripts/ (see §3).

---

## 3. Future plans

**Devcontainers / Docker reuse scripts/ (single source of truth)**

- **Goal:** Same flow for native dev and containers: Docker/devcontainers run `scripts/setup.sh` or `make setup-*` and `make &lt;target&gt;` or `scripts/build.sh` instead of duplicating logic (e.g. docker/windows/build_windows.sh).
- **Approach:** Workspace = repo root where possible; Dockerfile/entrypoint use scripts/setup.sh or make setup-windows; postCreateCommand/build use make windows or make windows-installer. Thin docker layer.
- **Windows Dockerfile fixes (before or as part of this):** See **§3.1** below (ninja-build, tar, QT_WINDOWS_DIR=win64_mingw, vcpkg abseil/utf8-range and ports alignment). A separate **docs/devcontainer_vs_setup_sanity_check.md** can be added to capture a diff/sanity checklist if useful.

**GitHub Actions: same workflow, cache images**

- **Goal:** All builds in CI use `make` + `scripts/`. Cache Docker images (especially Windows).
- **Approach:** Linux job — make setup-linux, make linux (optional linux-installer). Windows job — same image as devcontainer, make windows / make windows-installer in container; cache image. macOS job — make setup-macos, make macos (optional macos-installer). Wire artifacts and triggers.
- **Order:** Devcontainer reuse first, then add/refactor GitHub Actions and image cache.

**Auto-generate version from git**

- **Goal:** Single source of truth for version; no more hardcoded `2.0.6` across scripts and installers.
- **Source:** `git describe --tags --abbrev=4 --dirty --always` (e.g. `2.0.6`, `2.0.6-3-gabcd`, `2.0.6-dirty`). Optionally strip `-dirty` or normalize for installers that expect `x.y.z`.
- **Where `2.0.6` is currently hardcoded (search: `2.0.6`):**
  - **Packaging / installers:** **scripts/build_macos.sh** (Info.plist CFBundleShortVersionString, CFBundleVersion); **docker/windows/installer.nsi** (!define PRODUCT_VERSION); **docker/android/build_android.sh** and **docker/android/build_android_universal.sh** (android:versionName); **docker/linux/snap/snapcraft.yaml** (version).
  - **Source / runtime:** **src/game_defs.h** (POKERTH_BETA_RELEASE_STRING; comments for CLIENT_TYPE_QT_WIDGET and min version).
  - **Docs / examples:** **docker/linux/snap/README.md** (v2.0.6, pokerth_2.0.6_amd64.snap); **docker/linux/flatpak/README.md** (v2.0.6).
  - **Release process:** **ChangeLog** (version 2.0.6 header).
  - **Comments only (no automation needed):** **src/net/serverlobbythread.cpp** (pre-2.0.6); **src/game_defs.h** (inline comments).
- **Implementation:** Set e.g. `POKERTH_VERSION` in **scripts/functions.sh** from `git describe` (run from REPO_ROOT). Build/packaging scripts source it or receive it as env; CMake can define a version for **src/game_defs.h** or a generated header.
- **Order:** Can be done independently; useful before or with CI so artifacts get consistent version strings.

---

### 3.1 Devcontainers work (plan)

**Current state**

- **Windows:** `docker/windows/.devcontainer/` — Dockerfile installs deps (apt, vcpkg, aqtinstall, Qt, host Qt) into `ROOT=/opt/pokerth-windows`, clones pokerth into `${ROOT}/pokerth`, and `postCreateCommand` runs `build_windows.sh` from `docker/windows/`. Workspace is effectively `docker/windows` (compose mounts `../../windows` → `/workspaces/windows`). No use of `scripts/setup.sh` or `scripts/build.sh`; all logic duplicated in `docker/windows/build_windows.sh`.
- **Android:** `docker/android/.devcontainer/` — Uses docker-compose and a different image; `workspaceFolder` set to `/workspaces/${localWorkspaceFolderBasename}`. Not wired to shared scripts yet.
- **Repo root:** No root-level `.devcontainer`; opening from repo root does not use a devcontainer.

**Alignments needed (Windows) before/while reusing scripts**

- **ninja-build:** Install in Dockerfile so CMake can use Ninja if desired (scripts may assume it for generator).
- **tar:** Ensure `tar` is installed in image (needed by some toolchains/vcpkg).
- **QT_WINDOWS_DIR / aqt arch:** Scripts use `win64_mingw` for Qt 6.9+ and fall back to `mingw_64`. Dockerfile currently sets `QT_WINDOWS_DIR=${ROOT}/Qt/${QT_VERSION}/mingw_64` and installs with `win64_mingw` in the aqt command — inconsistent. Use one layout: either install with the arch aqt reports (e.g. `win64_mingw` for 6.9+) and set `QT_WINDOWS_DIR` to that path, or call `scripts/setup.sh` which already handles detection.
- **vcpkg ports:** Match **scripts/functions.sh** `VCPKG_PORTS`: add **abseil**, **utf8-range** (required for protobuf ≥ 5.27); remove **boost-uuid** if not in `VCPKG_PORTS`; keep list in sync so container and native use same deps.
- **VCPKG_DIR / QT_OUTPUT_DIR:** Scripts expect `VCPKG_DIR` (default `$HOME/vcpkg`) and `QT_OUTPUT_DIR` (default `$HOME/Qt`). In container, either set these to the image paths (e.g. `$ROOT/vcpkg`, `$ROOT/Qt`) or use a single layout (e.g. `$HOME/vcpkg`, `$HOME/Qt`) so `setup_linux_paths` and build work without extra overrides.

**Devcontainer reuse plan (concrete steps)**

1. **Single source of truth**
   - Treat `scripts/setup.sh` and `scripts/build.sh` (and `scripts/functions.sh`) as canonical. No duplicate install/build logic in Docker; Docker only installs minimal base (compiler, cmake, ninja, tar, python, git, etc.) and optionally runs `make setup-windows` (or `scripts/setup.sh` with `TARGET_PLATFORM=windows`) and then `make windows` / `make windows-installer`.

2. **Workspace = repo root**
   - Prefer one devcontainer at **repo root** (e.g. `.devcontainer/devcontainer.json`) that can build for Windows (and later Linux/Android if desired). Alternative: keep `docker/windows/.devcontainer` but have it mount the **repo root** as the workspace and run from there so `make` and `scripts/` are used.
   - Ensure `workspaceFolder` (or equivalent) is the repo root so `make` and `scripts/build.sh` see correct `REPO_ROOT` and paths.

3. **Windows image layout**
   - **Option A (recommended):** Dockerfile does **not** clone the repo; it only installs base system deps (mingw, cmake, ninja, tar, nsis, python, etc.). Then either:
     - **postCreateCommand:** run `make setup-windows` (or `TARGET_PLATFORM=windows scripts/setup.sh`) to install vcpkg + Qt via scripts (same as native), then optionally `make windows`.
     - Or split: **postCreateCommand** only runs setup; user runs `make windows` manually. This keeps image smaller and setup consistent with scripts.
   - **Option B:** Dockerfile still installs vcpkg and Qt itself (to match scripts' expected paths, e.g. `VCPKG_DIR`, `QT_OUTPUT_DIR`), but build step is always `make windows` / `scripts/build.sh` from repo root (no `build_windows.sh`). Env in Dockerfile must set `VCPKG_DIR`, `QT_OUTPUT_DIR`, and optionally `QT_WINDOWS_DIR` so they match what `setup_linux_paths` and build expect.

4. **devcontainer.json**
   - **postCreateCommand:** e.g. `make setup-windows` or `TARGET_PLATFORM=windows scripts/setup.sh` (if not done in Dockerfile), then optionally `make windows`. Do **not** call `build_windows.sh`.
   - **workspaceFolder:** repo root when opened from root; or document "Open folder: pokerth" so workspace is repo root.
   - **env:** Set `VCPKG_DIR`, `QT_OUTPUT_DIR` (and if needed `QT_WINDOWS_DIR`) in `containerEnv` so they match the image and scripts.

5. **Deprecate docker/windows/build_windows.sh**
   - Once devcontainer and CI use `make setup-windows` + `make windows` (or `scripts/build.sh`), remove or archive the duplicated logic in `docker/windows/build_windows.sh` and point any remaining references to `make`/scripts.

6. **Android / Linux devcontainers**
   - Same idea: base image + `make setup-*` and `make &lt;target&gt;` from repo root. Can be a follow-up after Windows devcontainer is switched over.

7. **Documentation**
   - **docs/building.md:** Add a short "Building in Dev Container (Windows)" section: open repo root in devcontainer, run `make setup-windows` once, then `make windows` or `make windows-installer`.
   - Optionally add **docs/devcontainer_vs_setup_sanity_check.md** with a checklist (ninja, tar, Qt path, vcpkg ports, env vars) for future regressions.

**Checklist before closing devcontainer work**

- [ ] Windows Dockerfile installs ninja-build, tar; vcpkg ports match `scripts/functions.sh` (abseil, utf8-range; no stray boost-uuid unless added to scripts).
- [ ] QT_WINDOWS_DIR / aqt arch consistent (win64_mingw vs mingw_64) and aligned with scripts.
- [ ] Workspace is repo root; postCreateCommand uses `make setup-windows` and `make windows` (or equivalent via scripts); no `build_windows.sh`.
- [ ] VCPKG_DIR / QT_OUTPUT_DIR (and QT_WINDOWS_DIR if needed) set in container so scripts find tools.
- [ ] building.md updated with devcontainer instructions.
- [ ] Optional: docs/devcontainer_vs_setup_sanity_check.md added.
