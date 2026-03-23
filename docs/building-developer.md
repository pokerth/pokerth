# Build system reference

Supplements **docs/building.md** and **INSTALL**. Covers refactor history, script roles, env, and Docker/devcontainer details for anyone working on the build system.

## Main focus: symmetric Docker kinds

**ensure_docker_deps.py** — per-kind **`_KIND_DEFAULT_VCPKG_TRIPLET`** and optional **`_KIND_EXTRA_READINESS_TRIPLETS`** (same **`vcpkg_ready`** probe for **`check_port`** on each; **Android** includes **`x64-linux`** because **setup_android.sh** installs host + target protobuf — see **Android: two protobuf installs and overlay hash** below). Qt (and for Android, SDK/NDK/Gradle) comes from the image; when ensure runs **setup.sh** for vcpkg refresh it passes **`SKIP_QT_INSTALL=yes`** and **`SKIP_SYSTEM_PACKAGES=yes`**. **devcontainer.json** **containerEnv**: **Windows** sets **QT_OUTPUT_DIR**; **Android** sets **VCPKG_DIR** (and the image sets **ROOT**). Stamps **`docker/<kind>/build/.stamp_setup`**. The container entrypoint runs as **vscode**; ensure runs **setup.sh** and **make** as that user. ensure sets **`IN_DOCKER=1`** only for the **make** invocation (Makefile uses it for **`REPO_BUILD_ROOT`**); Qt skip uses **`SKIP_QT_INSTALL`**.

---

## 1. What changed (pre-refactor → now)

- **Entry point:** Makefile at repo root is the only build/setup entry. No root-level `build_linux.sh` / `setup_windows.sh` etc.
- **Scripts:** All build/setup logic lives under **scripts/** (`build.sh`, `setup.sh`, `functions.sh`; macOS: `build_macos.sh`, `setup_macos.sh`). Linux and Windows share **setup.sh** / **build.sh**; **TARGET_PLATFORM** selects **linux** / **windows** / **android** (android → **setup_android.sh**). **REPO_ROOT** is set in `functions.sh`.
- **Windows:** **docker/windows/build_windows.sh** is deprecated (stderr warning; not for new use). Build: **scripts/build.sh** + **functions.sh**; **QT_VERSION** in **scripts/versions.env**.
- **Android:** **docker/android/.devcontainer/install_vcpkg_android.sh** deprecated the same way. vcpkg ports: **scripts/setup_android.sh** via **TARGET_PLATFORM=android scripts/setup.sh**, **ensure_docker_deps.py android**, **make setup-android** (host).
- **Docker / devcontainer:** **scripts/ensure_docker_deps.py** when vcpkg/protobuf missing runs **setup.sh** (then **make**). Windows devcontainer: **postCreateCommand** **ensure_docker_deps.py windows**. Windows vcpkg cache mount: **docker/windows/build/vcpkg** → `/opt/pokerth-windows/vcpkg` (Qt stays in image). Android vcpkg cache mount: **docker/android/build/vcpkg** → `/opt/pokerth-android/vcpkg`.
- **Reconfigure:** build.sh skips configure only when `CMakeCache.txt` exists and was for the **same repo path**. Broken build: `CLEAN=yes make <target>` or remove build dir (see building.md).
- **vcpkg:** **vcpkg_install_with_retry** in functions.sh retries once on "File exists" (interrupted copy).
- **Messages / docs:** Point to **make &lt;target&gt;** and **scripts/**.

---

## 2. How it works now

**Entry and scripts**

- **building.md** lists Makefile targets and platform flow.
- **functions.sh** — QT_VERSION, VCPKG_DIR, QT_OUTPUT_DIR, REPO_ROOT, VCPKG_PORTS, QT_MODULES.
- **setup.sh** — **linux** / **windows**: packages, Qt, vcpkg. **android** (early exit): **setup_android.sh** only (**VCPKG_DIR** required).
- **setup_android.sh** — Host: provision SDK/NDK/Gradle/Qt (when not skipped), then vcpkg + Android-triplet ports + protobuf overlay; writes **`.android_env`** under **`$BUILD_DIR`** and **`${ROOT}/.android_env`** when **ROOT** is set (Docker image / bind-safe paths).
- **build.sh** — **linux** / **windows**. **build_macos.sh** — macOS.

**Important env**

- **REPO_ROOT** — From functions.sh.
- **TARGET_PLATFORM** — **linux** | **windows** | **android** for setup.sh.
- **VCPKG_DIR**, **QT_OUTPUT_DIR**, **VCPKG_TRIPLET** (Android). **SKIP_QT_INSTALL** — when `yes`, setup.sh skips Qt install (Docker flows; Qt in image). Docker caches: **docker/windows/build/vcpkg**, **docker/android/build/vcpkg**.
- **Versions** — **scripts/versions.env**. Sync flatpak workflow with **FLATPAK_RUNTIME_VERSION**.

**Docker / devcontainers**

### Image build vs container run (different filesystems)

These are **not** the same step. Confusing them breaks expectations for vcpkg **`downloads/`**, clone location, and host persistence.

- **`docker build` / buildx** — Only the **Dockerfile** runs. **`devcontainer.json`** is **not** read for mounts or env. There are **no** host bind mounts during build. **`final`** **`RUN`** uses **`RUN --mount=type=cache,target=/opt/pokerth-<kind>/vcpkg/downloads,id=vcpkg-downloads-pokerth`** so vcpkg source archives persist in the **BuildKit cache** even when **`COPY scripts`** / the **`RUN setup*`** layer is re-executed. **Windows** and **Android** Dockerfiles share that **`id`** so overlapping port tarballs reuse one cache. Layer reuse still applies when inputs are unchanged; the cache mount adds cross-invalidation reuse for downloads only.

- **`docker run`** — **run_devcontainer.py** and VS Code apply **`devcontainer.json`**: **`mounts`**, **`containerEnv`**. Host **`docker/<kind>/build/vcpkg`** → **`/opt/pokerth-<kind>/vcpkg`**: the **entire** vcpkg tree (clone, **`downloads/`**, **`installed/`**, buildtrees, etc.). vcpkg’s default **`downloads/`** lives under that mount; no second bind for archives.

- **`make *-docker`** — **run_devcontainer.py** runs **`docker build`** when the image tag is missing or a forced rebuild is requested, then **`docker run`**. A path that works in the running container may be irrelevant during the build phase, and vice versa.

### Devcontainer flow (after the split above)

- **Windows:** Repo at `/workspaces/pokerth`. **ensure_docker_deps.py windows** → **setup.sh** (vcpkg in mounted cache; Qt from image) → **make windows**. Output **docker/windows/build/deploy/**.
- **make windows-docker** / **windows-docker-installer** — **run_devcontainer.py** + **ensure_docker_deps.py &lt;target&gt;**.
- **Android:** **make android-docker** → **ensure_docker_deps.py android** (may skip **setup.sh** if vcpkg/protobuf already ready) → **make android** → **scripts/build.sh** → **scripts/build_android.sh**. **make android** on Linux first ensures the setup stamp under **`$(REPO_BUILD_ROOT)`** (same value passed to **build.sh** as **`REPO_BUILD_ROOT`**; Docker **`IN_DOCKER=1`** uses **`docker/android/build`**). vcpkg root from **VCPKG_DIR** or **VCPKG_ROOT** (container). Host vcpkg prep: **make setup-android** with **VCPKG_DIR** (refreshes that stamp).
- **Fast repeat `make android-docker`:** **Gradle** runs **`assembleRelease`** only (incremental). Use **`CLEAN=yes make android-docker`** for **`gradlew clean assembleRelease`** when the APK/Gradle tree is broken. **OpenSSL** prebuilts under **`android-build/libs/$ARCH/`** are not re-downloaded when already present. **`run_devcontainer.py`** skips **`docker build`** when the image tag already exists (**default**); after Dockerfile or copied **scripts/** changes, run **`POKERTH_DOCKER_FORCE_BUILD=1 make android-docker`** or **`run_devcontainer.py --force-build …`**.
- **macOS + Docker:** Prefer Docker Desktop over Colima for **windows-docker** / **android-docker**.

**Devcontainer vs Dockerfile:** **devcontainer.json** is the single source for **host bind mounts** and **run** env (**`docker run`** / VS Code). **Dockerfile** **`final`** may use **BuildKit** **`RUN --mount=type=cache`** for vcpkg downloads at **`docker build`** (not expressible in **devcontainer.json**). Both Windows and Android use **devcontainer.json** only for compose (none). **run_devcontainer.py** and VS Code read the same **devcontainer** config for run; image build still follows the **Dockerfile**. Devcontainer vs setup sanity check (apt/Qt/vcpkg parity): [plans/android-setup-centralization.md](plans/android-setup-centralization.md#devcontainer-vs-setup-sanity-check).

**Maintainer responsibility (manual sync):** There is no generator tying **`docker/*/.devcontainer/Dockerfile`** to **`devcontainer.json`**. Whoever edits one is responsible for keeping the other consistent: **`RUN --mount=type=cache`** **`target`** (and any **`/opt/pokerth-*`** paths in **`ENV`/`ARG`/`RUN`**) must stay coherent with **`mounts`** **`target`** paths and **`containerEnv`** (**`VCPKG_DIR`**, **`QT_OUTPUT_DIR`**, **`ANDROID_CACHE_ROOT`**). Mismatches confuse **`make *-docker`** vs VS Code “Reopen in Container” and break “same path at build vs run” expectations.

**`build.options` vs `runArgs`:** [containers.dev](https://containers.dev) **`build.options`** is an array of extra arguments for **`docker build`**. **`runArgs`** is for **`docker run`**. **run_devcontainer.py** passes **`build.options`** to **`docker build`** when the array is non-empty; it always uses **`runArgs`** only for **`docker run`** (so platform flags needed at build time belong in **`build.options`**, not only in **`runArgs`**).

**Image build every `make *-docker`:** **run_devcontainer.py** skips **`docker build`** when **`docker image inspect <tag>`** succeeds (**default**, fast local iteration). It always runs **`docker build`** when the tag is missing (typical fresh CI runner). On **self-hosted** CI or any host where the tag may linger, set **`POKERTH_DOCKER_FORCE_BUILD=1`** so every job rebuilds the image. After Dockerfile or copied **scripts/** changes locally, use **`--force-build`** or the same env var. Layers **cache** when **`scripts/`** and **`docs/`** are unchanged; check **`DOCKER_BUILDKIT=1`** if builds behave oddly. Host **`CLEAN`** and **`BUILD_TARGET`** are forwarded into the container when set (e.g. **`CLEAN=yes make android-docker`**). Root **`.dockerignore`** trims build context size (build dirs, **`.git`**).

**Docker: bind mounts, runtime**

| Item | Windows | Android |
|------|---------|---------|
| **Build (`docker build`)** | **final** **`RUN`**: **`--mount=type=cache`** on **`/opt/pokerth-windows/vcpkg/downloads`**; vcpkg tree still under **`/opt/pokerth-windows/vcpkg`** in the layer. | **final** **`RUN`**: same cache mount pattern + **`setup_android.sh`**; SDK/NDK/Qt/vcpkg baked under **`/opt/pokerth-android`**. |
| **Run mounts (`docker run`)** | Repo → `/workspaces/pokerth`. **`docker/windows/build/vcpkg`** → **`/opt/pokerth-windows/vcpkg`** (full tree). **`QT_OUTPUT_DIR`** in **containerEnv**. | Repo mount + **`docker/android/build/vcpkg`** → **`/opt/pokerth-android/vcpkg`**. **ANDROID_CACHE_ROOT** in **containerEnv**. |
| **Run flow** | **ensure_docker_deps.py** → **setup.sh** → **make windows**. | **ensure_docker_deps.py** → **setup.sh**/**setup_android.sh** → **make android**. |

**Docker vcpkg and Qt (design)**

- **Goal:** One vcpkg pattern for Windows and Android: host **`docker/<kind>/build/vcpkg`** → container **`/opt/pokerth-<kind>/vcpkg`**. Bind-mount **only** that vcpkg directory so image paths for **Qt** (e.g. **`/opt/pokerth-windows/Qt`**), Android **SDK/NDK**, and pre-baked tooling are **not** shadowed by a mount over all of **`/opt/pokerth-*`**.
- **vcpkg downloads:** **Image build** — **`RUN --mount=type=cache`** on **`$VCPKG_DIR/downloads`** (BuildKit cache; shared **`id=vcpkg-downloads-pokerth`** across Windows/Android). **Container run** — archives and clone stay under the single **`…/build/vcpkg`** bind mount (**`$VCPKG_ROOT/downloads/`** by default). Re-cloning at run was tied to an **empty** mounted vcpkg dir (or first run), not a missing “download cache” mount.
- **`ensure_docker_deps.py`:** Per-kind root default **`/opt/pokerth-<kind>`**; treats **`$ROOT/vcpkg`** as vcpkg home. **Readiness** is one loop: primary **`VCPKG_TRIPLET`** plus any **`_KIND_EXTRA_READINESS_TRIPLETS`** for that kind (see **scripts/ensure_docker_deps.py**). It does **not** implement Windows-only Qt checks (that would break symmetry with Android).
- **Windows image:** The devcontainer image must include Qt (**Dockerfile** build ends at the **`final`** stage, not **`base`** only). Otherwise, when vcpkg is already ready, ensure skips **`setup.sh`** and the build can fail with missing Qt.
- **Android image:** Same pattern: **`final`** runs **setup_android.sh** so SDK/NDK/Gradle/Qt live under **`/opt/pokerth-android`**. **`build_android.sh`** sources repo **`.android_env`** first, then **`${ROOT}/.android_env`** when the repo mount hides the stamp tree.
- **Qt at ensure time:** For Docker, Qt lives in the image. ensure passes **`SKIP_QT_INSTALL=yes`** (and **`SKIP_SYSTEM_PACKAGES=yes`**) into **`setup.sh`** for vcpkg refresh; **`QT_OUTPUT_DIR`** comes from **`devcontainer.json`** **`containerEnv`** (or **`run_devcontainer.py`**), not from ensure.
- **Stale doc pitfall:** Container vcpkg is **`/opt/pokerth-<kind>/vcpkg`**, not **`/opt/pokerth-<kind>/build/vcpkg`** (CMake/build output stays under the repo / **`docker/<kind>/build`** as configured elsewhere).
- **Sanity checks:** To force a cold vcpkg setup, remove or empty **`docker/<kind>/build/vcpkg`**, then run **`make windows-docker`** / **`make android-docker`**. First run populates the host cache; second run should skip heavy **`setup.sh`** work. Confirm Qt/SDK/NDK still come from the image, not from accidental whole-tree mounts.

**Android: two protobuf installs and overlay hash**

- **setup_android.sh** installs **`protobuf:x64-linux`** first, then **`protobuf:${VCPKG_TRIPLET}`** with an overlay under **`$ROOT/vcpkg-overlays/protobuf`**. Overlay idempotency is hash-based: **`$ROOT/vcpkg-overlays/.protobuf_overlay_hash_${VCPKG_TRIPLET}`** stores the overlay content hash; protobuf reinstall runs when the hash changes or when triplet config files are missing.
- **`.stamp_setup`** in **`build_android/`** or **`docker/android/build/`** satisfies Make’s “setup ran” dependency but is **not** keyed by **VCPKG_TRIPLET**. Changing ABI/triplet on the host: remove that **`.stamp_setup`** (or **`make clean`**) and rerun **setup-android** / Docker ensure so ports for the new triplet run; the overlay hash file is triplet-specific.
- **ensure** uses **vcpkg** state (**VCPKG_TRIPLET** plus **`_KIND_EXTRA_READINESS_TRIPLETS`** for that kind) to decide whether to run **setup.sh** again; **`.stamp_setup`** can be touched by ensure before **make** — if something still looks wrong, treat emptying **`…/vcpkg`** or removing stamps as the hard reset.

**Targets**

- **make windows** — Linux host or macOS→Docker.
- **make android** — Linux host (**scripts/build_android.sh** via build.sh) or macOS→Docker.
- **make setup-android** — Host: **VCPKG_DIR** required; same vcpkg port path as Docker ensure.

---

## 3. Known issues

- **macOS Docker:** devcontainer **runArgs --platform linux/amd64** for Windows image.
- **Colima:** Unstable for **windows-docker** / **android-docker**; use Docker Desktop.
- **MinGW:** Dockerfile uses **posix** thread model (**x86_64-w64-mingw32-g++-posix**).

---

## 4. Planned Work

- **Cross-kind parity (Phase 6):** Broader alignment of base dirs + discovery, unified **build.sh** path; **`${ROOT}/.android_env`** already covers the bind-mount gap. See [android-setup-centralization.md](plans/android-setup-centralization.md).

---

## Out of scope for now

- **Version from git:** **POKERTH_VERSION** from **git describe** (see repo search for hardcoded **2.0.6**).
- **Android universal (fat APK):** **build_android_universal.sh**, **Dockerfile.universal** — location and integration TBD.
- **CI:** GitHub Actions + **make windows-docker** / **windows-docker-installer**; cache image.
- **Package-selection / Qt-install centralization:** Consider move from setup.sh into functions.sh (PKG variables kept in setup.sh by design).
- **Revisit legacy Qt 6.4 support:** Revert Qt 6.4 changes to settingsdialog.ui and settingsdialogimpl.cpp? Investigate.
- **Android vcpkg caches:** Whether to use separate host caches per ABI/triplet or one cache for all supported ABIs — TBD.
- **Windows persisted vcpkg:** Whether to add a stamp/version mechanism so a long-lived mounted vcpkg cannot go stale silently — TBD.
