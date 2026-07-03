## Build Android APK via Docker (arm64-v8a)

Builds the PokerTH QML APK for Android arm64 directly from the terminal, without entering the dev container.

### Prerequisites

- Docker installed and running
- Branch `qt6-qml` checked out
- Keystore for signing (one-time setup, see below)

### Usage

```bash
cd <project-root>
bash docker/android/build_android_arm64_docker.sh
```

Skip the Docker layer cache with `--no-cache`:

```bash
bash docker/android/build_android_arm64_docker.sh --no-cache
```

### Output

The unsigned APK is placed in `docker/android/`:

```
docker/android/android-build-release-unsigned.apk
```

### First Run – Build Time

The Docker image contains Qt, Android NDK, SDK and vcpkg.  
The first image build takes approximately **1 hour** (downloads + vcpkg compilation).  
Subsequent runs start within seconds thanks to the Docker layer cache, then proceed directly to the PokerTH build (~15 minutes).

### Build Artifacts

The CMake output is written locally to the project under:

```
build-android-arm64-v8a/
```

This directory can be safely deleted to force a clean rebuild.

---

## Sign the APK (outside Docker)

### Create a keystore (one-time)

```bash
keytool -genkey -v -keystore my.keystore -keyalg RSA -keysize 2048 -validity 10000 -alias app
```

Keep the keystore in a safe place — it is required for every future update.

### Sign the APK

```bash
cd docker/android/
apksigner sign --ks my.keystore --ks-key-alias app android-build-release-unsigned.apk
```

### Optional: zipalign before signing

```bash
cd docker/android/
zipalign -v 4 android-build-release-unsigned.apk PokerTH-arm64-release.apk
apksigner sign --ks my.keystore --ks-key-alias app PokerTH-arm64-release.apk
```

---

## Troubleshooting

**Rebuild the image from scratch:**
```bash
bash docker/android/build_android_arm64_docker.sh --no-cache
```

**Clean the build directory:**
```bash
rm -rf build-android-arm64-v8a/
```

**Remove the image manually:**
```bash
docker rmi pokerth-android-builder:arm64
```
