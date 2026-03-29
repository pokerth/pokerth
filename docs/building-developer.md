# Build system reference

Supplements **docs/building.md** and **INSTALL**. Covers refactor history, script roles, env, and Docker/devcontainer details for anyone working on the build system.

## Host directory convention

- **Non-Docker (native host `make`, no container):** Use **`build_<platform>/`** — e.g. **`build_linux/`**, **`build_windows/`**, **`build_android/`**, **`build_macos/`**. Stamps (**`build_<platform>/.stamp_setup`**), CMake trees, and deploy output for that flow stay under that directory (Makefile **`REPO_BUILD_ROOT`** when **`IN_DOCKER`** is unset).

- **Docker (all tooling that is Docker-related):** Use **`docker/<kind>/build/`** on the host for that kind (**`windows`**, **`android`**). Same tree for **vcpkg** (**`docker/<kind>/build/vcpkg`**), **Qt** (**`docker/<kind>/build/Qt`**), **stamps** (**`docker/<kind>/build/.stamp_setup`**), and **build/deploy/APK** outputs under **`docker/<kind>/build/...`**. **`make *-docker`** / devcontainer **`docker run`** binds **`…/vcpkg`** → **`/opt/pokerth-<kind>/vcpkg`** and **`…/Qt`** → **`/opt/pokerth-<kind>/Qt`**. **`docker build`** does **not** mount those paths; it uses the Dockerfile only — persisted caches and outputs land under **`docker/<kind>/build/`** once **`docker run`** / **`make`** inside the container writes there.

## Setup stages (conceptual) and when ensure_docker_deps.py runs

Three stages describe **what** **`setup_android.sh`** / **`setup_linux.sh`** (**`TARGET_PLATFORM=windows`** in Docker) do in order (one process; not three separate **`make`** steps):

1. **OS packages (apt / future brew)** — **`SKIP_SYSTEM_PACKAGES`** gates only this block. **Dockerfile** **base** may **`RUN apt-get`** from **`android-apt-packages.txt`** / **`windows-apt-packages.txt`**; native **`make setup-*`** runs the same lists when **`SKIP_SYSTEM_PACKAGES`** is unset (Linux **apt**).
2. **Toolchain / heavy deps (not apt)** — **Android:** **sdkmanager** (SDK/NDK), **Gradle**; **toolchain** also writes the Android manifest (**`.manifest.env`**, default basename in **`functions.sh`** as **`MANIFEST_ENV`**) under **`BUILD_DIR`** with Qt path hints from **`ROOT`** (no **aqt** here). Reading that file is **Android-only** (`setup_android.sh` / `build_android.sh`); other platforms do not use it. **Windows cross (Docker image):** MinGW / NSIS-related tooling as configured; **no** **aqt** or **vcpkg** install in the **`toolchain`** slice ( **`setup.sh toolchain`** in **`docker/windows/.devcontainer/Dockerfile`** **`final`**).
3. **deps** (at **`docker run`**, not in the Windows/Android **`final`** image **`RUN`**) — **Android:** **aqt** for Qt when **`Qt6Config.cmake`** is missing, then clone/bootstrap **`vcpkg`**, **`vcpkg install`** (Android **protobuf** overlay ports in this stage). **Windows:** **`setup_linux.sh`** with **`TARGET_PLATFORM=windows`** runs **Qt** (**aqt** when dirs missing) + **vcpkg** when **`setup.sh deps`** runs (e.g. from **`ensure_docker_deps.py`**).

**Optional layer argument (`setup.sh` … `all` \| `toolchain` \| `deps`):** **`setup_android.sh`** and **`setup_linux.sh`** ( **`TARGET_PLATFORM=windows`** ) implement **`toolchain` \| `deps` \| `all`**; Docker **Android** and **Windows** **`final`** images run **`setup.sh toolchain`** only. Native **Linux** full setup does not use the layer split the same way ( **`toolchain`/`deps`** on **linux** is an error—use **`all`**).

**`scripts/ensure_docker_deps.py`** — entry point for **Docker flows only**; it is **not** involved in plain **`make setup-android`** on a native host (you call **`setup.sh`** / **`make setup-android`** directly).

- **`make android-docker`** / **`make windows-docker`** — **`run_devcontainer.py`** passes **`DOCKER_GOAL`** (**`android`** or **`windows`**, not the **`*-docker`** name) into **`docker run … ./scripts/ensure_docker_deps.py android`** (or **`windows`**). **Ensure** treats **deps** as ready only when **both** hold: **vcpkg** has the required **protobuf** (primary triplet plus **`_KIND_EXTRA_READINESS_TRIPLETS`**, e.g. **`x64-linux`** for Android) **and** an executable **`qt-cmake`** exists under the mounted Qt tree (**`QT_OUTPUT_DIR`**, else **`${ROOT}/Qt`**). **If not**, it runs **`setup.sh deps`** (**`setup_android.sh`** / **`setup_linux.sh`** with **`TARGET_PLATFORM=windows`**); **apt** is skipped in the image because **`ENV SKIP_SYSTEM_PACKAGES=yes`**. It then touches the setup stamp and runs **`make`** with **`IN_DOCKER=1`**. If ensure skips **`setup.sh`** but the build still fails, refresh **deps** or empty **vcpkg**/**Qt** mounts as needed.
- **Windows devcontainer** — **`postCreateCommand`:** **`./scripts/ensure_docker_deps.py windows`**. That checks **`docker_deps_ready`**; if false, runs **`setup.sh deps`**; then **always** runs **`make windows`** (target is in **`kind_run_make_targets`** in **ensure**).
- **Android devcontainer** — no **`postCreateCommand`**; use **`make android-docker`** from the host (same ensure path) or run **`./scripts/ensure_docker_deps.py android`** yourself inside the container if you need that check before **`make android`**.

## Main focus: symmetric Docker kinds

**ensure_docker_deps.py** — per-kind **`_KIND_DEFAULT_VCPKG_TRIPLET`** and optional **`_KIND_EXTRA_READINESS_TRIPLETS`** (same **`vcpkg_ready`** probe for **`check_port`** on each; **Android** includes **`x64-linux`** because **setup_android.sh** installs host + target protobuf — see **Android: two protobuf installs and overlay hash** below). **“System packages”** means **only** what OS package managers install (**apt** / **brew** lists such as **android-apt-packages.txt**): that is what **`SKIP_SYSTEM_PACKAGES`** gates. **Not** system packages in this sense: **sdkmanager** (SDK/NDK), **pip** / **aqt** (Qt), **Gradle**, **vcpkg** — **setup** still runs those when ensure invokes **`setup.sh`** (no separate **`SKIP_*`** for Qt). **`SKIP_SYSTEM_PACKAGES=yes`** is **Dockerfile** **`ENV`** when **base** already ran the apt list; native **`make setup-*`** leaves it unset so **apt** runs on Linux. A future **macOS** Dockerfile could **`brew install`** the same role. **devcontainer.json** **containerEnv**: **Windows** sets **QT_OUTPUT_DIR**; **Android** sets **VCPKG_DIR** (and the image sets **ROOT**). Stamps **`docker/<kind>/build/.stamp_setup`**. The container entrypoint runs as **vscode**; ensure runs **setup.sh** and **make** as that user. ensure sets **`IN_DOCKER=1`** only for the **make** invocation (Makefile uses it for **`REPO_BUILD_ROOT`**). Qt installs are **idempotent** (**Android** **deps** / **Windows** **`install_qt_for_platform`** directory checks), not gated by a **`SKIP_QT_INSTALL`** env var (removed).

---

## 1. What changed (pre-refactor → now)

- **Entry point:** Makefile at repo root is the only build/setup entry. No root-level `build_linux.sh` / `setup_windows.sh` etc.
- **Scripts:** Entry points **scripts/setup.sh** and **scripts/build.sh** dispatch by **TARGET_PLATFORM**; implementations: **setup_linux.sh** / **build_linux.sh** (linux and windows on a Linux host), **setup_android.sh** / **build_android.sh**, **setup_macos.sh** / **build_macos.sh**. Shared **functions.sh**; **REPO_ROOT** from `functions.sh`.
- **Windows:** Use **build.sh** → **build_linux.sh** (not **docker/windows/build_windows.sh**, which only prints a redirect message). **QT_VERSION** in **scripts/versions.env**.
- **Android:** vcpkg ports: **setup_android.sh** via **setup.sh** (**TARGET_PLATFORM=android**), **ensure_docker_deps.py android**, **make setup-android** (host). Ignore **docker/android/.devcontainer/install_vcpkg_android.sh** — same work is done through **setup_android.sh**.
- **Docker / devcontainer:** **ensure_docker_deps.py** when **vcpkg** or **Qt** readiness fails runs **`setup.sh deps`** (then **make**). Windows devcontainer: **postCreateCommand** **ensure_docker_deps.py windows**. **docker run** bind: host **`docker/<kind>/build/vcpkg`** and **`…/Qt`** → **`/opt/pokerth-<kind>/{vcpkg,Qt}`** (SDK/Android toolchain paths per **containerEnv** / image).
- **Reconfigure:** **build_linux.sh** (via **build.sh**) skips configure only when `CMakeCache.txt` exists and was for the **same repo path**. Broken build: `CLEAN=yes make <target>` or remove build dir (see building.md).
- **vcpkg:** **vcpkg_install_with_retry** in functions.sh retries once on "File exists" (interrupted copy).
- **Messages / docs:** Point to **make &lt;target&gt;** and **scripts/**.

---

## 2. How it works now

**Entry and scripts**

- **building.md** lists Makefile targets and platform flow.
- **functions.sh** — QT_VERSION, VCPKG_DIR, QT_OUTPUT_DIR, REPO_ROOT, VCPKG_PORTS, QT_MODULES.
- **setup.sh** — Dispatcher: **linux** / **windows** → **setup_linux.sh** (packages, Qt, vcpkg); **android** → **setup_android.sh**; **macos** → **setup_macos.sh**. No optional repo-local include file (overrides use **`TARGET_PLATFORM`**, **`make setup-*`**, and documented env vars).
- **setup_linux.sh** — On a Linux host: native **linux** or **windows** cross (MinGW, aqt, vcpkg), linuxdeployqt for linux.
- **setup_android.sh** — Host: **toolchain** = SDK/NDK/Gradle + **`${MANIFEST_ENV}`** (Qt path hints from **`ROOT`** / env, no **aqt**); **deps** = Qt (**aqt** when **`Qt6Config.cmake`** is missing) then **vcpkg** ports (including **protobuf** overlay); writes **`${MANIFEST_ENV}`** under **`${REPO_ROOT}/${BUILD_DIR}`**. The **deps** layer and **`build_android.sh`** read **`${REPO_ROOT}/${BUILD_DIR}/${MANIFEST_ENV}`** or **`${ROOT}/${MANIFEST_ENV}`** (Docker **`_sync_manifest_to_root`**).
- **build.sh** — Dispatcher: **linux** / **windows** → **build_linux.sh**; **android** → **build_android.sh**; **macos** → **build_macos.sh**.

**Important env**

- **REPO_ROOT** — From functions.sh.
- **TARGET_PLATFORM** — **linux** | **windows** | **android** for setup.sh.
- **VCPKG_DIR**, **QT_OUTPUT_DIR**, **VCPKG_TRIPLET** (Android). Docker **run**: host **`docker/<kind>/build`** bind-mounted at **`/opt/pokerth-<kind>`** (vcpkg at **`…/vcpkg`**). Qt setup is **stage + idempotent checks** (no **`SKIP_QT_INSTALL`**).
- **SKIP_SYSTEM_PACKAGES** — **apt** / **brew** OS package lists only (**SKIP** the **`apt-get`** / equivalent block in **setup**). Unset on a **native** Linux host so **`android-apt-packages.txt`** / **`windows-apt-packages.txt`** run. **Dockerfile** **`ENV yes`** when **base** already installed that list. Does **not** skip **NDK**, **Qt**, **pip**, **Gradle**, or **vcpkg**.
- **Versions** — **scripts/versions.env**. Sync flatpak workflow with **FLATPAK_RUNTIME_VERSION**.

### Environment variable reference (setup / build / Docker)

Canonical inventory for **Android + Windows** Docker flows and shared **ensure** behavior. Stage is **`setup.sh`** argv **`toolchain` \| `deps` \| `all`** (**`LAYER`**); **`SKIP_*`** for Qt/vcpkg is intentionally gone (**`SKIP_QT_INSTALL`** removed).

| Variable | Set by | Role | Notes |
| --- | --- | --- | --- |
| **`TARGET_PLATFORM`** | **`Makefile`**, user, **`ensure`** (via **`setup.sh`** env) | Dispatches **`setup_linux.sh`** / **`setup_android.sh`** / … | Required for **`setup.sh`**. |
| **`LAYER`** | **`setup.sh`** argv | **Stage** for Android/Windows | **`linux`** + **`LAYER≠all`** is an error in **`setup_linux.sh`**. |
| **`SKIP_SYSTEM_PACKAGES`** | Default **`no`**; **`ENV yes`** in Android/Windows **`Dockerfile`** when **base** already ran **`android-apt-packages.txt`** / **`windows-apt-packages.txt`** | Gates **only** OS **apt** blocks in **`setup_android.sh`** / **`setup_linux.sh`** | **Kept** — orthogonal to **`LAYER`**. Does **not** skip NDK/Qt/vcpkg. |
| **`USE_AQT`** / **`USE_VCPKG`** | **`functions.sh`**; **`TARGET_PLATFORM=windows`** forces both **`yes`** | Linux/Windows setup | See **`functions.sh`**. |
| **`VCPKG_DIR`**, **`VCPKG_ROOT`**, **`VCPKG_TRIPLET`** | Env, **`functions.sh`**, **`ensure`** | vcpkg location and triplet | **`ensure`** uses **`VCPKG_ROOT`** or **`$ROOT/vcpkg`**. |
| **`BUILD_DIR`** | **`Makefile`** stamp recipes | Stamps, **`REPO_BUILD_ROOT`**, Android manifest path | Must match **`ensure`** for Docker (**`docker/<kind>/build`**). |
| **`ROOT`** | **`devcontainer.json`**, **`ensure`** default **`/opt/pokerth-<kind>`** | Docker bind root; Android manifest mirror | |
| **`QT_OUTPUT_DIR`** | **`functions.sh`** (default **`$HOME/Qt`**), **Windows** **`devcontainer.json`** | Qt tree; **`qt_cmake_ready`** in **ensure** | |
| **`MANIFEST_ENV`** | **`functions.sh`** (default **`.manifest.env`**) | Android manifest basename | |
| **`ANDROID_CACHE_ROOT`** | **`setup_android.sh`** | SDK/NDK/Gradle/venv | |
| **`IN_DOCKER`** | **`ensure_docker_deps.py`** → **`make`** | **`REPO_BUILD_ROOT=docker/$(TARGET_PLATFORM)/build`** | |
| **`CLEAN`** | User, **`functions.sh`** | **Build** only (e.g. drops **`android-build`**) | Not a setup stage. |
| **`CREATE_INSTALLER`** | User, **`build_*.sh`** | AppImage / NSIS / DMG | Build path only. |
| **`SETUP_ALREADY_DONE`** | **`Makefile`** stamp recipes | Non-empty → **`touch`** stamp only, no **`setup.sh`** | Not read by shell scripts. |
| **`POKERTH_DOCKER_FORCE_BUILD`** | User, **`run_devcontainer.py`** | Force **`docker build`** | |

**Docker / devcontainers**

### Image build vs container run (different filesystems)

These are **not** the same step. Confusing them breaks expectations for vcpkg **`downloads/`**, clone location, and host persistence.

- **`docker build` / buildx** — The **Dockerfile** **`final`** **`RUN`** uses plain **`RUN`** (no **`RUN --mount`**). **Android:** **`setup.sh toolchain`** bakes SDK/NDK/Gradle into the image under **`/opt/pokerth-android`**. **Windows:** **`setup.sh toolchain`** bakes MinGW-related **toolchain** prep only under **`/opt/pokerth-windows`**; **Qt** and **vcpkg** populate under **`docker run`** ( **`ensure_docker_deps`** → **`setup.sh deps`** ). Host **`docker/<kind>/build`** is **not** bound during **`docker build`**; vcpkg **`downloads/`** for **run** use the mounted host dirs below.

- **`docker run`** — **run_devcontainer.py** and VS Code apply **`devcontainer.json`**: **`mounts`**, **`containerEnv`**. Host **`docker/<kind>/build/vcpkg`** → **`/opt/pokerth-<kind>/vcpkg`**, **`docker/<kind>/build/Qt`** → **`/opt/pokerth-<kind>/Qt`** (SDK/NDK/toolchain paths under **`/opt/pokerth-<kind>`** stay in the image unless overwritten by these submounts).

### Host vs container ownership (bind mounts)

Bind-mounted **`docker/<kind>/build/{vcpkg,Qt,...}`** is written from a **Linux container user** (often **`vscode`**, uid **1000**). On the host the same paths are often owned by your login uid (e.g. **501** on macOS) with mode **`755`**. Inside the container, uid **1000** may not be the directory owner, so writes count as “other” and fail with **`Permission denied`** (**`.manifest.env`**, stamps, **vcpkg**/**Qt**) even when **`ls` on the host looks fine**.

**Mitigations (prefer earlier):**

- Align **container uid/gid** with the **host user** for workspace and cache (Docker Desktop / Linux / CI features differ).
- Avoid running **`setup`** / **`make`** as **root** on bind-mounted paths (avoids **root-owned** files on the host).
- **Recovery:** On the host, **`chown -R "$(id -u):$(id -g)" docker/<kind>/build`** (or the stuck subtree); fix stale **root-owned** **`.stamp_setup`** / manifest the same way.
- **CI:** Job user should own the workspace cache or match the container user.
- **`touch_stamp_file`** in **ensure** helps **unwritable stamps** only; it does not fix an unwritable cache directory — treat that as **environment**, not **`chmod 777`** in the repo.

- **`make *-docker`** — **run_devcontainer.py** runs **`docker build`** when the image tag is missing or a forced rebuild is requested, then **`docker run`**. A path that works in the running container may be irrelevant during the build phase, and vice versa.

### Devcontainer flow (after the split above)

- **Windows:** Repo at `/workspaces/pokerth`. **ensure_docker_deps.py windows** → **`setup.sh deps`** when **vcpkg** or **Qt** (**`qt-cmake`**) readiness fails (mounted **vcpkg**/**Qt** cache; **`docker build`** only ran **toolchain**) → **make windows**. Output **docker/windows/build/deploy/**.
- **make windows-docker** / **windows-docker-installer** — **run_devcontainer.py** + **ensure_docker_deps.py &lt;target&gt;**.
- **Android:** **make android-docker** → **ensure_docker_deps.py android** (may skip **setup.sh** if **vcpkg** and **Qt** are already ready) → **make android** → **build.sh** → **build_android.sh**. **make android** on Linux first ensures the setup stamp under **`$(REPO_BUILD_ROOT)`** (same value passed to **build.sh** as **`REPO_BUILD_ROOT`**; Docker **`IN_DOCKER=1`** uses **`docker/android/build`**). vcpkg root from **VCPKG_DIR** or **VCPKG_ROOT** (container). Host vcpkg prep: **make setup-android** with **VCPKG_DIR** (refreshes that stamp).
- **Fast repeat `make android-docker`:** **`androiddeployqt`** is invoked with **`--release`** (Gradle **`assembleRelease`** inside Qt). Use **`CLEAN=yes make android-docker`** to remove **`$(REPO_BUILD_ROOT)/android-build`** when the APK/Gradle tree is stale (same intent as the former **`gradlew clean assembleRelease`**). **OpenSSL** prebuilts under **`android-build/libs/$ARCH/`** are not re-downloaded when already present. **`run_devcontainer.py`** skips **`docker build`** when the image tag already exists (**default**); after Dockerfile or copied **scripts/** changes, run **`POKERTH_DOCKER_FORCE_BUILD=1 make android-docker`** or **`run_devcontainer.py --force-build …`**.
- **macOS + Docker:** Prefer Docker Desktop over Colima for **windows-docker** / **android-docker**.
- **`${localWorkspaceFolder}`:** **devcontainer.json** **`workspaceMount`** and **`mounts`** treat **`localWorkspaceFolder`** as the **repository root** (same substitution **run_devcontainer.py** uses). Persistent binds are **`docker/<kind>/build/vcpkg`** and **`docker/<kind>/build/Qt`** onto **`/opt/pokerth-<kind>/{vcpkg,Qt}`**. Open the **repo root** in VS Code/Cursor when using Dev Containers so paths match **`make *-docker`**.

**Devcontainer vs Dockerfile:** **devcontainer.json** is the single source for **host bind mounts** and **run** env (**`docker run`** / VS Code). **`docker build`** does **not** use those binds; the **Dockerfile** **`final`** stage runs **`setup.sh toolchain`** in the image filesystem only (**deps** at **run**). **run_devcontainer.py** and VS Code read the same **devcontainer** config for run; image build still follows the **Dockerfile**. Devcontainer vs setup sanity check (apt/Qt/vcpkg parity): [plans/android-setup-centralization.md](plans/android-setup-centralization.md#devcontainer-vs-setup-sanity-check).

**Maintainer responsibility (manual sync):** There is no generator tying **`docker/*/.devcontainer/Dockerfile`** to **`devcontainer.json`**. Whoever edits one is responsible for keeping the other consistent: bind **`source`** / **`target`** for vcpkg and **`/opt/pokerth-*`** paths in **`ENV`/`ARG`/`RUN`** must match **`mounts`** and **`containerEnv`** (**`VCPKG_DIR`**, **`QT_OUTPUT_DIR`**, **`ANDROID_CACHE_ROOT`**). Mismatches confuse **`make *-docker`** vs VS Code “Reopen in Container” and break “same path at build vs run” expectations.

**`build.options` vs `runArgs`:** [containers.dev](https://containers.dev) **`build.options`** is an array of extra arguments for **`docker build`**. **`runArgs`** is for **`docker run`**. **run_devcontainer.py** passes **`build.options`** to **`docker build`** when the array is non-empty; it always uses **`runArgs`** only for **`docker run`** (so platform flags needed at build time belong in **`build.options`**, not only in **`runArgs`**).

**Image build every `make *-docker`:** **run_devcontainer.py** skips **`docker build`** when **`docker image inspect <tag>`** succeeds (**default**, fast local iteration). It always runs **`docker build`** when the tag is missing (typical fresh CI runner). On **self-hosted** CI or any host where the tag may linger, set **`POKERTH_DOCKER_FORCE_BUILD=1`** so every job rebuilds the image. After Dockerfile or copied **scripts/** changes locally, use **`--force-build`** or the same env var. Android/Windows devcontainer **Dockerfiles** **`COPY scripts`** only (image layers invalidate when **`scripts/`** changes; **`patches/`** lives under **`scripts/patches/`**). Check **`DOCKER_BUILDKIT=1`** if builds behave oddly. Host **`CLEAN`** and **`BUILD_TARGET`** are forwarded into the container when set (e.g. **`CLEAN=yes make android-docker`**). Root **`.dockerignore`** trims build context size (build dirs, **`.git`**).

**Docker: bind mounts, runtime**

| Item | Windows | Android |
|------|---------|---------|
| **Build (`docker build`)** | **final** **`RUN`**: plain **`RUN`** (no **`RUN --mount`**): **`./scripts/setup.sh toolchain`** (**TARGET_PLATFORM=windows** → **setup_linux.sh**) — MinGW-related **toolchain** only under **`/opt/pokerth-windows`** (apt skipped via **`SKIP_SYSTEM_PACKAGES`**). **Qt** + **vcpkg** are **not** baked in **`final`**. | **final**: one **`RUN`**: **`./scripts/setup.sh toolchain`** only (SDK/NDK/Gradle + **`.manifest.env`** in the image). **Deps** (**aqt**, vcpkg) run at **`docker run`** via **`ensure_docker_deps`** → **`setup.sh deps`** when needed. |
| **Run mounts (`docker run`)** | Repo → `/workspaces/pokerth`. **`docker/windows/build/vcpkg`** → **`/opt/pokerth-windows/vcpkg`**, **`docker/windows/build/Qt`** → **`/opt/pokerth-windows/Qt`**. **`QT_OUTPUT_DIR`** in **containerEnv**. | Repo mount + **`docker/android/build/vcpkg`** → **`/opt/pokerth-android/vcpkg`**, **`docker/android/build/Qt`** → **`/opt/pokerth-android/Qt`** (**ANDROID_CACHE_ROOT** / **ROOT** in **containerEnv**). |
| **Run flow** | **ensure_docker_deps.py** → **`setup.sh deps`** (if **vcpkg** or **Qt** not ready) → **make windows**. | **ensure_docker_deps.py** → **`setup.sh deps`** (if **vcpkg** or **Qt** not ready) → **make android**. |

**Docker vcpkg and Qt (design)**

- **Goal:** At **`docker run`**, host **`docker/<kind>/build/vcpkg`** and **`…/Qt`** bind onto **`/opt/pokerth-<kind>/{vcpkg,Qt}`**. **`docker build`** does not use those binds; **Android** bakes **SDK/NDK** into the image; **Windows** **`docker build`** bakes **toolchain** only—**Qt** and **vcpkg** populate from **run** (mount + **`setup.sh deps`**).
- **vcpkg downloads:** **`downloads/`** lives under **`docker/<kind>/build/vcpkg/downloads`** on the host when using the devcontainer mounts (cold **Windows**/**Android** **`deps`** fills the mount, not **`docker build`** layers).
- **`ensure_docker_deps.py`:** Per-kind root default **`/opt/pokerth-<kind>`**; treats **`$ROOT/vcpkg`** as vcpkg home. **`docker_deps_ready()`** requires **both** **`vcpkg_deps_ready()`** (primary **`VCPKG_TRIPLET`** plus **`_KIND_EXTRA_READINESS_TRIPLETS`**) **and** **`qt_cmake_ready()`** (executable **`qt-cmake`** under **`QT_OUTPUT_DIR`** or **`${ROOT}/Qt`**). Same rule for **Android** and **Windows**.
- **Windows image:** The **`final`** stage must include the **toolchain** (**MinGW**, etc.). **Qt** and **vcpkg** live under the mounted cache; if either side fails readiness, **`ensure`** runs **`setup.sh deps`**.
- **Android image:** **`final`** runs **`toolchain`** only; **deps** run at **`docker run`**. **`build_android.sh`** sources **`${REPO_BUILD_ROOT}/${MANIFEST_ENV}`** first, then **`${ROOT}/${MANIFEST_ENV}`** ( **`_sync_manifest_to_root`** in **`setup_android.sh`**). APK packaging: **`androiddeployqt --release`**; **`@drawable/ic_launcher`** — see **docs/building.md** (Android); **`ic_launcher.png`** in package source dirs is a **symlink** to **`data/gfx/gui/misc/windowicon_transparent.png`**; **`build_android.sh`** stages with **`cp -L`** into **`android-build`**.
- **Qt at ensure time:** For Docker, **Qt** is expected under the mounted cache (or installed when **`setup.sh deps`** runs). ensure does **not** set **`SKIP_QT_INSTALL`** (removed). **`setup.sh deps`** runs when **`docker_deps_ready`** is false (**vcpkg** or **Qt**); **`SKIP_SYSTEM_PACKAGES`** (apt only) comes from image **`ENV`**, not from ensure. **`QT_OUTPUT_DIR`** comes from **`devcontainer.json`** **`containerEnv`** (or **`run_devcontainer.py`**), not from ensure.
- **Stale doc pitfall:** Container vcpkg is **`/opt/pokerth-<kind>/vcpkg`**, not **`/opt/pokerth-<kind>/build/vcpkg`** (CMake/build output stays under the repo / **`docker/<kind>/build`** as configured elsewhere).
- **Sanity checks:** To force a cold vcpkg setup, remove or empty **`docker/<kind>/build/vcpkg`**, then run **`make windows-docker`** / **`make android-docker`**. First run populates the host cache; second run should skip heavy **`setup.sh`** work. Confirm **Android** SDK/NDK still come from the image where intended; **Windows** **Qt**/vcpkg come from **deps** + mounts, not from accidental whole-tree mounts hiding **`/opt/pokerth-<kind>`**.

**Android: two protobuf installs and overlay hash** (part of the **deps** / vcpkg stage above)

- **setup_android.sh** installs **`protobuf:x64-linux`** first, then **`protobuf:${VCPKG_TRIPLET}`** with an overlay under **`$ROOT/vcpkg-overlays/protobuf`**. Overlay idempotency is hash-based: **`$ROOT/vcpkg-overlays/.protobuf_overlay_hash_${VCPKG_TRIPLET}`** stores the overlay content hash; protobuf reinstall runs when the hash changes or when triplet config files are missing.
- **`.stamp_setup`** in **`build_android/`** or **`docker/android/build/`** satisfies Make’s “setup ran” dependency but is **not** keyed by **VCPKG_TRIPLET**. Changing ABI/triplet on the host: remove that **`.stamp_setup`** (or **`make clean`**) and rerun **setup-android** / Docker ensure so ports for the new triplet run; the overlay hash file is triplet-specific.
- **ensure** uses **vcpkg** state (**VCPKG_TRIPLET** plus **`_KIND_EXTRA_READINESS_TRIPLETS`** for that kind) to decide whether to run **setup.sh** again; **`.stamp_setup`** can be touched by ensure before **make** — if something still looks wrong, treat emptying **`…/vcpkg`** or removing stamps as the hard reset.

**Targets**

- **make windows** — Linux host or macOS→Docker.
- **make android** — Linux host (**build_android.sh** via **build.sh**) or macOS→Docker.
- **make setup-android** — Host: **VCPKG_DIR** required; same vcpkg port path as Docker ensure.

---

## 3. Known issues

- **macOS Docker:** devcontainer **runArgs --platform linux/amd64** for Windows image.
- **Colima:** Unstable for **windows-docker** / **android-docker**; use Docker Desktop.
- **MinGW:** Dockerfile uses **posix** thread model (**x86_64-w64-mingw32-g++-posix**).

---

## 4. Planned Work

- **Cross-kind parity (Phase 6):** Broader alignment of base dirs + discovery, unified **build.sh** / **build_linux.sh** path; **`${REPO_ROOT}/${BUILD_DIR}/${MANIFEST_ENV}`** (default **`.manifest.env`**) is the canonical Android setup manifest path. See [android-setup-centralization.md](plans/android-setup-centralization.md).
- **`.stamp_setup` / readiness in one place:** Today **Makefiles** and **`scripts/ensure_docker_deps.py`** both touch setup stamps / readiness logic in overlapping ways — partly because **Docker** runs **setup** paths (**`setup.sh`**, image **`Dockerfile`**) **without** going through **`make`**, so stamp semantics cannot live only in the Makefile. **To do (later):** consolidate or clearly layer “setup complete” signaling so there is a single contract (what creates **`docker/<kind>/build/.stamp_setup`**, when **`ensure_docker_deps`** may skip **`setup.sh`**, and how that relates to **`make`** rules).

---

## Out of scope for now

- **Version from git:** **POKERTH_VERSION** from **git describe** (see repo search for hardcoded **2.0.6**).
- **Android universal (fat APK):** **build_android_universal.sh**, **Dockerfile.universal** — location and integration TBD.
- **CI:** GitHub Actions + **make windows-docker** / **windows-docker-installer**; cache image.
- **Package-selection / Qt-install centralization:** Consider move from **setup_linux.sh** into functions.sh (PKG variables kept in **setup_linux.sh** by design).
- **Revisit Qt 6.4 support:** Revert Qt 6.4 changes to settingsdialog.ui and settingsdialogimpl.cpp? Investigate.
- **Android vcpkg caches:** Whether to use separate host caches per ABI/triplet or one cache for all supported ABIs — TBD.
- **Windows persisted vcpkg:** Whether to add a stamp/version mechanism so a long-lived mounted vcpkg cannot go stale silently — TBD.
