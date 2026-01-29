# Build guide

**Top-level entry:** Use the **Makefile**. Run `make help` for all targets.

- **Default `make`:** On Linux runs `make linux`; on macOS runs `make macos`. So you can run **`make`** after setup and get the native build.
- **Default `make setup`:** On Linux runs `make setup-linux`; on macOS runs `make setup-macos`. Run **`make setup`** once to install dependencies for the current OS.
- **Stamp files:** When you run `make linux`, `make windows`, or `make macos`, Make checks for a setup stamp (`.stamp-setup-linux`, `.stamp-setup-windows`, or `.stamp-setup-macos`). If the stamp is missing, it runs the corresponding setup first, then builds. So **`make`** or **`make linux`** (etc.) can be run without having run setup beforehand; setup will run once and create the stamp.
- **`make clean`:** Removes `build_linux/`, `build_windows/`, `build_macos/` and the setup stamp files. After that, the next `make` or `make linux` (etc.) will run setup again.

## Summary by platform

| Platform | Setup | Build | Build dir | Notes |
|----------|--------|-------|-----------|--------|
| **Linux** (native) | `make setup-linux` or `make setup` (on Linux) | `make linux` or `make` (on Linux) | `build_linux/` | Qt 6.4.2+ (e.g. Ubuntu 24.04 system Qt). Deploy: `build_linux/deploy/` (binary + data). |
| **Windows** (cross from Linux) | `make setup-windows` | `make windows` | `build_windows/` | MinGW + vcpkg + Qt (aqtinstall). Output: `build_windows/deploy/`. |
| **macOS** | `make setup-macos` or `make setup` (on macOS) | `make macos` or `make` (on macOS) | `build_macos/` | Homebrew, vcpkg, Qt via aqtinstall. Produces `.app` bundle. |

- **Clean rebuild:** `make clean` (removes build dirs and stamps) or `CLEAN=yes make linux` (or make windows / make macos) wipes the build dir and reconfigures from scratch.
- **Create installer:** `make linux-installer`, `make windows-installer`, or `make macos-installer` — Linux: AppImage (linuxdeployqt); Windows: NSIS (makensis); macOS: DMG.
- **Usage:** Pass any argument (e.g. `--help`) to a setup or build script to print usage and exit.
- **Build targets:** `pokerth_client` (default), `pokerth_qml-client`, `pokerth_dedicated_server`, `pokerth_official_server`, `pokerth_chatcleaner`. Override with `BUILD_TARGET=…`.

**Makefile targets:** `make`, `make setup`, `make linux`, `make windows`, `make macos`, `make setup-linux`, `make setup-windows`, `make setup-macos`, `make clean`, `make help`. Installers: `make linux-installer`, `make windows-installer`, `make macos-installer`, `make installers`. Pass env: `CLEAN=yes make linux`.

---

## Linux

1. **Setup (once):** `make setup-linux` or `make setup` (on Linux). Or run `./scripts/setup.sh` (may need sudo). If you skip this, **`make linux`** or **`make`** will run setup automatically (creates `.stamp-setup-linux` when done).
2. **Build:** `make linux` or `make` (on Linux) → binary in `build_linux/bin/` and deploy dir in `build_linux/deploy/`.
3. **Run:** `cd build_linux/deploy && ./pokerth_client` or `./build_linux/bin/pokerth_client` (bin has `data` pointing to repo data).
4. **Optional:** `make linux-installer` or `CREATE_INSTALLER=yes ./scripts/build.sh` runs linuxdeployqt to create an AppImage (deploy dir is always created; this only adds the AppImage step). **make setup-linux** installs linuxdeployqt to `~/bin` if missing; ensure `~/bin` is in your PATH (e.g. `export PATH="$HOME/bin:$PATH"` in `.bashrc`). To install manually, see [linuxdeployqt releases](https://github.com/probonopd/linuxdeployqt/releases).

**Deploy directory:** Like Windows, every Linux build creates `build_linux/deploy/` with the binary and `deploy/data` pointing to repo data (no copy). `build_linux/bin/data` also points to repo data so running from `bin/` works.

**Script behavior:** Reuses `build_linux/` by default. Configure is skipped only if `CMakeCache.txt` and `build.ninja` exist; if `build.ninja` is missing (e.g. partial or copied build), the script reconfigures automatically.

**Ubuntu vs Debian:** Qt 6.4.2+ is required. Ubuntu **24.04** has Qt 6.4.2 (fine; use system Qt). Ubuntu **22.04** has Qt 6.2.x — too old; use **USE_AQT=yes make setup-linux** on 22.04 to install Qt 6.4.2+ via aqtinstall, then you can build and create the AppImage there. If you see “Could not find … Qt6 … 6.7.0”, run `CLEAN=yes make linux` to pick up the lower minimum.

**linuxdeployqt "host system is too new":** linuxdeployqt only runs on systems with glibc ≤ 2.35 (e.g. Ubuntu 22.04 Jammy) so the AppImage works on older distros. On a newer host (e.g. Ubuntu 24.04) it will error. Workarounds: (1) Build the AppImage in an **Ubuntu 22.04** container — 22.04 system Qt is too old (6.2.x), so use **USE_AQT=yes make setup-linux** then **make linux** and **make linux-installer** in the container (repo mounted), or (2) build and run from the deploy dir on your host (`make linux` then `build_linux/deploy/pokerth_client`) and skip the AppImage. See [linuxdeployqt #340](https://github.com/probonopd/linuxdeployqt/issues/340).

**After upgrading from Ubuntu 22.04 to 24.04:** Build entry is unchanged: use the **Makefile** (`make setup-linux`, `make linux`, `make linux-installer`). Ubuntu 24.04 has Qt 6.4.2+ (fine for building). **make linux-installer** (AppImage) fails on 24.04 ("host system too new"); use the workarounds above (run from deploy dir, or build the AppImage in a 22.04 container with **USE_AQT=yes** so Qt 6.4.2+ is used). On 22.04, system Qt (6.2.x) is too old — use USE_AQT=yes there too.

---

## Windows (cross-compile from Linux)

1. **Setup (once):** `make setup-windows` (or `TARGET_PLATFORM=windows ./scripts/setup.sh`). If you skip this, **`make windows`** will run setup automatically (creates `.stamp-setup-windows` when done).
2. **Build:** `make windows` → `build_windows/deploy/` is populated with the exe, Qt/MinGW DLLs, `data/`, plugins, and `qt.conf`.
3. Copy **`build_windows/deploy`** to Windows and run `pokerth_client.exe` from that directory, or run **`make windows-installer`** to create an NSIS installer. Requires `makensis` (e.g. `apt install nsis`).

**Script behavior:** Same reuse logic as Linux; when reusing, configure is skipped if cache and generator files exist. Windows deploy dir is always created for Windows builds.

**Host vs Docker:** You can build Windows binaries and the NSIS installer either **on the host** (`make windows` / `make windows-installer` with MinGW, vcpkg, Qt, makensis) or **inside Docker** (`docker/windows/build_windows.sh`). Both use the same **Windows packaging assets** in `docker/windows/` (installer.nsi, pokerth.ico, and the created PokerTH-*-Setup.exe). So `docker/windows/` is shared: not Docker-only.

### Required files in deploy

- **Executable:** `pokerth_client.exe`
- **Qt DLLs:** Qt6Core, Qt6Gui, Qt6Widgets, Qt6Network, Qt6Sql, Qt6Xml, Qt6Multimedia
- **MinGW runtime:** libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll
- **Qt plugins:** `plugins/platforms/qwindows.dll` (required)
- **Config:** `qt.conf` (paths to plugins)
- **Data:** `data/` (stylesheets, icons, sounds, translations)

### Deploy layout

```
build_windows/deploy/
├── pokerth_client.exe
├── Qt6*.dll, libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll
├── qt.conf
├── data/
└── plugins/platforms/qwindows.dll
```

### Testing on Windows

1. Copy entire `build_windows/deploy` to Windows (or run **`make windows-installer`** to create an NSIS installer).
2. Ensure all DLLs are next to the exe and `plugins/platforms/qwindows.dll` and `data/` are present.
3. Run `pokerth_client.exe`.

**Troubleshooting:** See **windows_troubleshooting.md** (e.g. 0xc0000022, DEP, antivirus, missing DLLs). For build failures on Linux before copying, see “Building on Linux” above and “Build Script Behavior” in the dev context below. **Ubuntu:** If setup fails during vcpkg install with “unreachable code was reached” / “Download failed”, see the “Windows cross-compile on Ubuntu” section below.

### Windows cross-compile on Ubuntu (vcpkg bug)

**What’s wrong with Ubuntu:** Nothing is wrong with Ubuntu itself. **vcpkg** (the C++ package manager used for Windows cross-compile) has a bug in its metrics/telemetry code (`metrics.cpp`). After a successful download it hits “unreachable code was reached” and then reports “Download failed”. That path is triggered on **Ubuntu** (e.g. different glibc/toolchain) but not on **Debian**, so the same setup works on Debian and fails on Ubuntu. It’s a vcpkg bug, not an Ubuntu bug.

**Workaround:** Do the Windows setup and build on **Debian** (same host or in a container). For example: use a Debian machine or VM for `make setup-windows` and `make windows`, then copy `build_windows/deploy` to Windows; or use Docker with a Debian image and run setup/build inside the container. Once vcpkg fixes the issue upstream, Ubuntu will work without changes.

**OpenSSL MinGW (SIO_UDP_NETRESET):** When building OpenSSL for the MinGW triplet, OpenSSL’s QUIC code uses the Windows constant `SIO_UDP_NETRESET`, which is not defined in older MinGW headers. The setup script automatically patches vcpkg’s OpenSSL port to add `no-quic` for MinGW so the build succeeds. The patch is in `docs/patches/vcpkg-openssl-mingw-no-quic.patch` and is applied once when you run setup with a mingw triplet.

---

## macOS

1. **Setup (once):** `make setup-macos` or `make setup` (on macOS). Or run `./scripts/setup_macos.sh`. Requires Xcode and Xcode command line tools. If you skip this, **`make macos`** or **`make`** will run setup automatically (creates `.stamp-setup-macos` when done).
2. **Build:** `make macos` or `make` (on macOS) → builds into `build_macos/` and creates `build_macos/PokerTH.app`.
3. **Optional:** `make macos-installer` or `CREATE_INSTALLER=yes ./scripts/build_macos.sh` creates a DMG installer after the app bundle.

**Script behavior:** Reuses `build_macos/` by default; `CLEAN=yes make macos` for a full reconfigure. Uses vcpkg for Boost, Protobuf, etc., and Qt from aqtinstall (e.g. 6.9.2).

---

## Script roles

- **Top-level entry:** Use **make** (see `make help`). Scripts live in **scripts/** (`scripts/build.sh`, `scripts/setup.sh`, `scripts/build_macos.sh`, `scripts/setup_macos.sh`, `scripts/clean_build.sh`).
- **Default goals:** On Linux, `make` = `make linux` and `make setup` = `make setup-linux`. On macOS, `make` = `make macos` and `make setup` = `make setup-macos`.
- **Stamp files:** `.stamp-setup-linux`, `.stamp-setup-windows`, `.stamp-setup-macos` are created when the corresponding setup finishes. Build targets (`make linux`, `make windows`, `make macos`) depend on these stamps; if a stamp is missing, Make runs the setup first, then the build. So you can run **`make`** or **`make linux`** (etc.) without having run setup beforehand.
- **Setup scripts** (`scripts/setup.sh`, `scripts/setup_macos.sh`): Install dependencies only (packages, vcpkg, Qt). Do not build.
- **Build scripts** (`scripts/build.sh`, `scripts/build_macos.sh`): Check that dependencies exist, then configure and build. If you use the Makefile, setup is run automatically when the stamp is missing.
- **make clean:** Runs `scripts/clean_build.sh` (removes `build_linux/`, `build_windows/`, `build_macos/`) and deletes the setup stamp files. Next `make` or `make linux` (etc.) will run setup again. Or use `CLEAN=yes make linux` (or make windows / make macos) to only wipe the build dir and reconfigure.
- **create_serverlist.sh** (root, legacy): Uses `./build/bin/zlib_compress`. Run from repo root after building (e.g. `make linux`). If you built in a different dir, build there or symlink.

## Build script context (for developers)

- **scripts/build.sh** (Linux host; Linux and Windows target)
  - **Target:** Set `TARGET_PLATFORM=linux` or `TARGET_PLATFORM=windows`; the Makefile sets it (e.g. `make windows`).
  - **Reuse:** Does not delete build dir unless `CLEAN=yes`.
  - **Configure skipped:** Only if `CMakeCache.txt` exists and (for Linux) `build.ninja` exists; otherwise script reconfigures.
  - **Linux:** Always populates `build_linux/deploy/` with binary and `deploy/data` → `../../data`; also creates `bin/data` → `../../data` so running from `bin/` finds data. `make linux-installer` runs linuxdeployqt for AppImage.
  - **Windows:** Always populates `build_windows/deploy/`; `chmod +x` on all `.dll`. `make windows-installer` runs NSIS (requires `makensis`).
- **scripts/setup.sh**
  - **Target:** Set `TARGET_PLATFORM=linux` or `TARGET_PLATFORM=windows`; the Makefile sets it (e.g. `make setup-windows`).
- **scripts/functions.sh**
  - **REPO_ROOT:** Set to repo root (parent of `scripts/` when sourced from `scripts/`).
  - **Windows cross-compile:** `QT_HOST_PATH` = `$QT_OUTPUT_DIR/$QT_VERSION/gcc_64`.
  - **SETUP_SCRIPT / BUILD_SCRIPT:** `scripts/setup.sh`, `scripts/build.sh` for usage messages.
  - **Usage:** Any argument to a setup or build script prints usage and exits.
- **scripts/build_macos.sh**
  - Does not install any utilities; run `scripts/setup_macos.sh` first. Reuse and `CLEAN=yes make macos` same idea as Linux. `make macos-installer` creates DMG installer.

**Refactor history and future plans:** See **docs/building-future.md** (what changed vs pre-refactor, how it works for devs, planned devcontainer/CI work).
