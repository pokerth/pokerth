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

## 2. How things work (developer-oriented)

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
- **Windows Dockerfile fixes (before or as part of this):** See **docs/devcontainer_vs_setup_sanity_check.md** (ninja-build, tar, QT_WINDOWS_DIR=win64_mingw, vcpkg abseil/utf8-range and ports alignment).

**GitHub Actions: same workflow, cache images**

- **Goal:** All builds in CI use `make` + `scripts/`. Cache Docker images (especially Windows).
- **Approach:** Linux job — make setup-linux, make linux (optional linux-installer). Windows job — same image as devcontainer, make windows / make windows-installer in container; cache image. macOS job — make setup-macos, make macos (optional macos-installer). Wire artifacts and triggers.
- **Order:** Devcontainer reuse first, then add/refactor GitHub Actions and image cache.
