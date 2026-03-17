# Build guide

How to build PokerTH (for contributors and packagers). **Top-level entry:** Use the **Makefile**. Run `make help` for all targets.

- **Default `make`:** On Linux runs `make linux`; on macOS runs `make macos`. So you can run **`make`** after setup and get the native build.
- **Default `make setup`:** On Linux runs `make setup-linux`; on macOS runs `make setup-macos`. Run **`make setup`** once to install dependencies for the current OS.
- **Stamp files:** When you run `make linux`, `make windows`, or `make macos`, Make checks for a setup stamp (`.stamp-setup-linux`, `.stamp-setup-windows`, or `.stamp-setup-macos`). If the stamp is missing, it runs the corresponding setup first, then builds. So **`make`** or **`make linux`** (etc.) can be run without having run setup beforehand; setup will run once and create the stamp.
- **`make clean`:** Removes `build_linux/`, `build_windows/`, `build_macos/`, `docker/windows/build/` and the setup stamp files. After that, the next `make` or `make linux` (etc.) will run setup again.

## Summary by platform

| Platform | Setup | Build | Build dir | Notes |
|----------|--------|-------|-----------|--------|
| **Linux** (native) | `make setup-linux` or `make setup` (on Linux) | `make linux` or `make` (on Linux) | `build_linux/` | Qt 6.4.2+ (e.g. Ubuntu 24.04 system Qt). Deploy: `build_linux/deploy/` (binary + data). |
| **Windows** (cross from Linux; or Docker on macOS) | `make setup-windows` (Linux) or none (macOS: Docker) | `make windows` | `build_windows/` or `docker/windows/build/` (Docker) | Linux: MinGW + vcpkg + Qt. macOS: **make windows** runs in Docker. Local output: `build_windows/deploy/`. Docker: **make windows-docker** → `docker/windows/build/deploy/`. |
| **macOS** | `make setup-macos` or `make setup` (on macOS) | `make macos` or `make` (on macOS) | `build_macos/` | Homebrew, vcpkg, Qt via aqtinstall. Produces `.app` bundle. |
| **Android** | none (Docker) | `make android` | `build-android-<arch>/` | Via Docker on Linux and macOS. APK in `build-android-<arch>/android-build/build/outputs/apk/release/`. |

- **Clean rebuild:** `make clean` (removes build dirs and stamps) or `CLEAN=yes make linux` (or make windows / make macos) wipes the build dir and reconfigures from scratch. If you see "Makefile not found", "Configure incomplete", or cache/toolchain errors, run `CLEAN=yes make <target>` and retry.
- **Create installer:** `make linux-installer`, `make windows-installer`, or `make macos-installer` — Linux: AppImage (linuxdeployqt); Windows: NSIS (makensis); macOS: DMG.
- **Usage:** Pass any argument (e.g. `--help`) to a setup or build script to print usage and exit.
- **Build targets:** `pokerth_client` (default), `pokerth_qml-client`, `pokerth_dedicated_server`, `pokerth_official_server`, `pokerth_chatcleaner`. Override with `BUILD_TARGET=…`.

**Makefile targets:** `make`, `make setup`, `make linux`, `make windows`, `make windows-docker`, `make macos`, `make android`, `make setup-linux`, `make setup-windows`, `make setup-macos`, `make clean`, `make help`. Installers: `make linux-installer`, `make windows-installer`, `make windows-installer-docker`, `make macos-installer`, `make installers`. Pass env: `CLEAN=yes make linux`.

**Windows vs Android (Docker):** **`make windows`** means “produce a Windows build”; on Linux it uses the host toolchain (MinGW, vcpkg, Qt), on macOS it uses Docker (no host Windows toolchain). So the *method* depends on the host. To always use Docker for Windows, use **`make windows-docker`** or **`make windows-installer-docker`**. **`make android`** always means “build for Android in Docker” — there is no host Android build path, so the meaning is unambiguous.

---

## Linux

1. **Setup (once):** `make setup-linux` or `make setup` (on Linux). Or run `./scripts/setup.sh` (may need sudo). If you skip this, **`make linux`** or **`make`** will run setup automatically (creates `.stamp-setup-linux` when done).
2. **Build:** `make linux` or `make` (on Linux) → binary in `build_linux/bin/` and deploy dir in `build_linux/deploy/`.
3. **Run:** `cd build_linux/deploy && ./pokerth_client` or `./build_linux/bin/pokerth_client` (bin has `data` pointing to repo data).
4. **Optional:** `make linux-installer` or `CREATE_INSTALLER=yes ./scripts/build.sh` runs linuxdeployqt to create an AppImage (deploy dir is always created; this only adds the AppImage step). **make setup-linux** installs linuxdeployqt to `~/bin` if missing; ensure `~/bin` is in your PATH (e.g. `export PATH="$HOME/bin:$PATH"` in `.bashrc`). To install manually, see [linuxdeployqt releases](https://github.com/probonopd/linuxdeployqt/releases). linuxdeployqt needs Qt6’s **qmake** to find plugins; the build script prefers Qt6 qmake over system `/usr/bin/qmake` (Qt5). If you see “qmake: could not exec … No such file or directory”, install Qt6 qmake (e.g. **`apt install qmake6`** or **`qt6-tools-dev`**) so the script can pass it to linuxdeployqt.

**Deploy directory:** Like Windows, every Linux build creates `build_linux/deploy/` with the binary and `deploy/data` pointing to repo data (no copy). `build_linux/bin/data` also points to repo data so running from `bin/` works.

**Script behavior:** Reuses `build_linux/` by default; reconfigure only on path change or if you run `CLEAN=yes` / remove the build dir (see Summary above).

**Ubuntu vs Debian:** Qt 6.4.2+ is required. Ubuntu **24.04** has Qt 6.4.2 (fine; use system Qt). Ubuntu **22.04** has Qt 6.2.x — too old; use **USE_AQT=yes make setup-linux** on 22.04 to install Qt 6.4.2+ via aqtinstall, then you can build and create the AppImage there. If you see “Could not find … Qt6 … 6.7.0”, run `CLEAN=yes make linux` to pick up the lower minimum.

**linuxdeployqt "host system is too new":** linuxdeployqt only runs on systems with glibc ≤ 2.35 (e.g. Ubuntu 22.04 Jammy) so the AppImage works on older distros. On a newer host (e.g. Ubuntu 24.04) it will error. **For testing only:** run **`LINUXDEPLOYQT_ALLOW_NEW_GLIBC=yes make linux-installer`** to pass `-unsupported-allow-new-glibc`; the AppImage may not run on older distros. Other workarounds: (1) Build the AppImage in an **Ubuntu 22.04** container — use **USE_AQT=yes make setup-linux** then **make linux** and **make linux-installer** in the container (repo mounted), or (2) build and run from the deploy dir on your host (`make linux` then `build_linux/deploy/pokerth_client`) and skip the AppImage. See [linuxdeployqt #340](https://github.com/probonopd/linuxdeployqt/issues/340).

**After upgrading from Ubuntu 22.04 to 24.04:** Build entry is unchanged: use the **Makefile** (`make setup-linux`, `make linux`, `make linux-installer`). Ubuntu 24.04 has Qt 6.4.2+ (fine for building). **make linux-installer** (AppImage) fails on 24.04 ("host system too new"); use the workarounds above (run from deploy dir, or build the AppImage in a 22.04 container with **USE_AQT=yes** so Qt 6.4.2+ is used). On 22.04, system Qt (6.2.x) is too old — use USE_AQT=yes there too.

---

## Windows (cross-compile from Linux, or via Docker on macOS)

1. **On Linux:** **Setup (once):** `make setup-windows` (or `TARGET_PLATFORM=windows ./scripts/setup.sh`). If you skip this, **`make windows`** will run setup automatically (creates `.stamp-setup-windows` when done).
2. **On macOS:** **`make windows`** and **`make windows-installer`** run the Windows build inside Docker (same as **`make windows-docker`** / **`make windows-installer-docker`**). No host setup required; Docker is required.
3. **Build:** `make windows` → `build_windows/deploy/` is populated with the exe, Qt/MinGW DLLs, `data/`, plugins, and `qt.conf`.
4. Copy **`build_windows/deploy`** to Windows and run `pokerth_client.exe` from that directory, or run **`make windows-installer`** to create an NSIS installer.

**NSIS (makensis):** Required only for **`make windows-installer`** (or `CREATE_INSTALLER=yes`). We assume it is already installed: on Linux run **`make setup-windows`** (which installs the `nsis` package), or install it yourself (e.g. **`apt install nsis`**). In the Windows Docker image, **`scripts/setup.sh`** installs nsis as part of the base packages. **scripts/build.sh** does not install nsis; it fails with a clear message if `makensis` is missing.

**Script behavior:** Same reuse logic as Linux (reconfigure on path change or `CLEAN=yes`; see Summary). If broken on host: `CLEAN=yes make windows`. If broken in Docker: remove `docker/windows/build` then `make windows-docker`. Launchers `pokerth_launcher.bat` and `run_pokerth.sh` are copied from **scripts/**.

**Host vs Docker:** You can build Windows binaries and the NSIS installer either **on the host** (`make windows` → `build_windows/deploy/`) or **via Docker** (`make windows-docker` → `docker/windows/build/deploy/`). Docker uses `docker/windows/build/` so local and Docker do not share the same tree. All use the same **Windows packaging assets** in `docker/windows/` (installer.nsi, pokerth.ico).

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

**Troubleshooting:** See **docs/windows_troubleshooting.md** (e.g. 0xc0000022, DEP, antivirus, missing DLLs). For build failures on Linux before copying, see the **Linux** section above and **docs/building-developer.md**. **Ubuntu:** If setup fails during vcpkg install with “unreachable code was reached” / “Download failed”, see the “Windows cross-compile on Ubuntu” section below. **Docker:** vcpkg and Qt live in **`docker/windows/vcpkg/`**; you can inspect **`docker/windows/vcpkg/vcpkg/buildtrees/`** for vcpkg build logs (e.g. `*/install-*-out.log`) if setup fails.

### Building in Dev Container (Windows)

Open the **docker/windows** folder in Cursor/VS Code so the Windows devcontainer (**docker/windows/.devcontainer/**) is used; the repo root is mounted at `/workspaces/pokerth`. The devcontainer matches **`make windows-docker`**: **base** image, **`docker/windows/vcpkg`** mount, and **`WINDOWS_BUILD_SUBDIR=docker/windows/build`**. After first create, run **`make windows`** or **`make windows-installer`**; output is in **`docker/windows/build/deploy/`**. See **docker/windows/README_windows.md** for prerequisites.

### Windows cross-compile on Ubuntu (vcpkg bug)

**What’s wrong with Ubuntu:** Nothing is wrong with Ubuntu itself. **vcpkg** (the C++ package manager used for Windows cross-compile) has a bug in its metrics/telemetry code (`metrics.cpp`). After a successful download it hits “unreachable code was reached” and then reports “Download failed”. That path is triggered on **Ubuntu** (e.g. different glibc/toolchain) but not on **Debian**, so the same setup works on Debian and fails on Ubuntu. It’s a vcpkg bug, not an Ubuntu bug.

**Workaround:** Do the Windows setup and build on **Debian** (same host or in a container). For example: use a Debian machine or VM for `make setup-windows` and `make windows`, then copy `build_windows/deploy` to Windows; or use Docker with a Debian image and run setup/build inside the container. Once vcpkg fixes the issue upstream, Ubuntu will work without changes.

**OpenSSL MinGW (SIO_UDP_NETRESET):** When building OpenSSL for the MinGW triplet, OpenSSL’s QUIC code uses the Windows constant `SIO_UDP_NETRESET`, which is not defined in older MinGW headers. The setup script automatically patches vcpkg’s OpenSSL port to add `no-quic` for MinGW so the build succeeds. The patch is in `docs/patches/vcpkg-openssl-mingw-no-quic.patch` and is applied once when you run setup with a mingw triplet.

---

## macOS

1. **Setup (once):** `make setup-macos` or `make setup` (on macOS). Or run `./scripts/setup_macos.sh`. Requires Xcode and Xcode command line tools. If you skip this, **`make macos`** or **`make`** will run setup automatically (creates `.stamp-setup-macos` when done).
2. **Build:** `make macos` or `make` (on macOS) → builds into `build_macos/` and creates `build_macos/PokerTH.app`.
3. **Optional:** `make macos-installer` or `CREATE_INSTALLER=yes ./scripts/build_macos.sh` creates a DMG installer after the app bundle.

**Script behavior:** Reuses `build_macos/` by default; `CLEAN=yes make macos` for a full reconfigure. Uses vcpkg for Boost, Protobuf, etc., and Qt from aqtinstall (default **QT_VERSION** 6.9.3 in `scripts/functions.sh`).

---

## Android (via Docker, Linux and macOS)

1. **Build:** From the repo root run **`make android`**. The Makefile builds the Android devcontainer image (first run can take a long time), mounts the repo, and runs **`make android-in-docker`** inside the container (which runs **docker/android/build_android.sh**). Requires Docker; no host setup.
2. **Output:** Unsigned APK at **`build-android-<arch>/android-build/build/outputs/apk/release/android-build-release-unsigned.apk`** (default arch is set in **docker/android/.devcontainer/Dockerfile**, e.g. `arm64-v8a`).
3. **Other architectures:** Run **`make android ANDROID_BUILD_ARGS="--arch armeabi-v7a"`** or **`ANDROID_BUILD_ARGS="--arch x86_64"`** etc. See **docker/android/README_android.md** for signing and devcontainer usage.

---

## Script roles

- **Top-level entry:** Use **make** (see `make help`). Scripts live in **scripts/** (`build.sh`, `setup.sh`, `build_macos.sh`, `setup_macos.sh`, `clean_build.sh`).
- **Default goals:** On Linux, `make` = `make linux` and `make setup` = `make setup-linux`. On macOS, `make` = `make macos` and `make setup` = `make setup-macos`.
- **Stamp files:** `.stamp-setup-linux`, `.stamp-setup-windows`, `.stamp-setup-macos` are created when setup finishes; if missing, Make runs setup first, then the build.
- **Setup scripts:** Install deps only (packages, vcpkg, Qt). **Build scripts:** Configure and build; deploy dir is always created. **make clean:** Removes build dirs and stamps; next `make` will run setup again. Or `CLEAN=yes make <target>` to only wipe the build dir and reconfigure.
- **create_serverlist.sh** (root, legacy): Expects `./build/bin/zlib_compress`. Binary is at `build_linux/bin/zlib_compress`; symlink `build` → `build_linux` or run it manually.

**Build system details (scripts, env, reconfigure, Docker):** **docs/building-developer.md**.
