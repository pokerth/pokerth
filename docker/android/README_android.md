## Prerequisites

- Docker
- VS Code with Dev Containers extension (ms-vscode-remote.remote-containers)

## Build Instructions

**Command line (recommended):** From **repo root**, **`make android-docker`**. **scripts/run_devcontainer.py** reads **docker/android/.devcontainer/devcontainer.json** (same as VS Code): **`build.options`** (e.g. **`--platform=linux/amd64`**) is passed to **`docker build`**; **`runArgs`** to **`docker run`**.

Flow: container starts → **scripts/ensure_docker_deps.py android** — if protobuf/vcpkg are not ready under the mounted tree, it runs **scripts/setup.sh** (**TARGET_PLATFORM=android**) → **scripts/setup_android.sh** (ports into **`docker/android/build/vcpkg`** via mount), sets **BUILD_DIR** to **`docker/android/build`** so **`.android_env`** and **`.stamp_setup`** stay aligned with **REPO_BUILD_ROOT** — then **`make android`** → **scripts/build_android.sh**.

**Mounts:** Repo → **`/workspaces/pokerth`**. **`docker/android/build/vcpkg`** → **`/opt/pokerth-android/vcpkg`** (SDK/NDK/Qt stay from the image).

**Image:** The Dockerfile **`final`** stage runs **setup_android.sh** so **`/opt/pokerth-android`** already has SDK/NDK/Gradle/Qt; ensure still runs **setup_android**-backed **setup.sh** when the bind-mounted vcpkg needs ports.

**APK (Docker):** **`docker/android/build/android-build/build/outputs/apk/release/`** (unsigned). **Host** **`make android`:** **`build_android/android-build/...`** instead.

Other ABIs: **`make android ANDROID_BUILD_ARGS="--arch x86_64"`** (**scripts/build_android.sh --help**). Align **VCPKG_TRIPLET** with the ABI when pre-seeding vcpkg (see **docs/building-developer.md**).

**Host Linux:** **`export VCPKG_DIR=build_android/vcpkg`** (example) then **`make setup-android`** — full SDK/NDK/Gradle/Qt + vcpkg (same script path as Docker ensure), then **`make android`**.

**Deprecated (do not run):** **docker/android/.devcontainer/install_vcpkg_android.sh** — reference only.

**VS Code Dev Container:** Open **docker/android** in VS Code → **Dev Containers: Rebuild and Reopen in Container**. Repo at **`/workspaces/pokerth`**. With toolchain in the image, **`make android`** is usually enough once vcpkg is populated; use **`make setup-android`** if you need to refresh ports (**VCPKG_DIR** must point at your vcpkg root). **`make android-docker`** from the host uses the same **devcontainer.json**.

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

Sign the APK (Docker output path; use **`build_android/...`** for a host tree):

```bash
apksigner sign --ks my.keystore --ks-key-alias app \
  docker/android/build/android-build/build/outputs/apk/release/android-build-release-unsigned.apk
```
