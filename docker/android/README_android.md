## Prerequisites

- Docker
- VS Code with Dev Containers extension (ms-vscode-remote.remote-containers)

## Build Instructions

**From the command line (Linux and macOS):** From the **repo root** run **`make android`**. This builds the devcontainer image (if needed), runs the Android build inside the container with the repo mounted, and leaves the APK in **`build-android-<arch>/android-build/build/outputs/apk/release/`**. For other architectures: **`make android ANDROID_BUILD_ARGS="--arch armeabi-v7a"`** or **`ANDROID_BUILD_ARGS="--arch x86_64"`** (see docker/android/build_android.sh --help). Requires Docker only.

**Using the VS Code Dev Container:** Open the **docker/android** folder, then Rebuild and Reopen in Container. Before building, edit the Dockerfile in `.devcontainer` to set the architecture (e.g. `ANDROID_ARCH=arm64-v8a`). You may need to edit docker-compose.yml for network settings. Inside the container the workspace is the repo; run:

```bash
bash docker/android/build_android.sh
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
apksigner sign --ks my.keystore --ks-key-alias app ${ROOT}/pokerth/build-android-${ANDROID_ARCH}/android-build/build/outputs/apk/release/android-build-release-unsigned.apk
```

