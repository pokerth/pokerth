## Prerequisites

- Docker
- VS Code with Dev Containers extension (ms-vscode-remote.remote-containers)

## Build Instructions

**Command line (recommended):** From **repo root**, **`make android-docker`**. Flow: **`scripts/ensure_docker_deps.py android`** → **`scripts/setup.sh`** (**TARGET_PLATFORM=android**) → **`scripts/setup_android.sh`** (vcpkg + protobuf into **`docker/android/build`**), then **`make android`**. In Docker, `ensure_docker_deps.py` makes the stamp location consistent with Windows by using **`docker/android/build/.stamp_setup`** (via `ANDROID_BUILD_DIR` override). Two binds: **`build_android`** (workspace dev convenience) and **`docker/android/build/vcpkg`** → **`/opt/pokerth-android/vcpkg`** (SDK/NDK/Qt in image). APK under **`build-android-<arch>/android-build/.../release/`**. Other ABIs: **`ANDROID_BUILD_ARGS="--arch x86_64"`** etc. (**scripts/build_android.sh --help**). Align **`VCPKG_TRIPLET`** with the ABI when pre-seeding vcpkg (see **docs/building-developer.md**).

**Host Linux (vcpkg only):** **`export VCPKG_DIR=…`** then **`make setup-android`** — same port install as Docker ensure before **`make android`**.

**Deprecated (do not run):** **`docker/android/.devcontainer/install_vcpkg_android.sh`** — like **`docker/windows/build_windows.sh`**; reference-only.

**Using the VS Code Dev Container:** Open **docker/android** in VS Code → **Dev Containers: Rebuild and Reopen in Container**. The repo is mounted at `/workspaces/pokerth`. Run **`make setup-android`** then **`make android`** to build. Or use **`make android-docker`** from the repo root for an all-in-one flow.

```bash
make setup-android
make android
```

The unsigned APK will be at:
`build-android-${ANDROID_ARCH}/android-build/build/outputs/apk/release/android-build-release-unsigned.apk`

## Sign the APK

Generate keystore (first time only):
```bash
keytool -genkey -v -keystore my.keystore -keyalg RSA -keysize 2048 -validity 10000 -alias app
```

Sign the APK:
```bash
apksigner sign --ks my.keystore --ks-key-alias app build-android-${ANDROID_ARCH}/android-build/build/outputs/apk/release/android-build-release-unsigned.apk
```

