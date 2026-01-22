## Prerequisites

- Docker
- VS Code with Dev Containers extension (ms-vscode-remote.remote-containers)

## Build Instructions

Best practice is to use the VS Code Dev-Container feature.

Before building the container image, edit Dockerfile in `.devcontainer` folder and set architecture and target to build for.
Supported architectures: `arm64-v8a`, `armeabi-v7a`, `x86_64`

You might also need to edit docker-compose.yml for network settings.

Inside the running container:

```bash
cd ${ROOT}/pokerth
bash docker/android/build_android.sh
```

The unsigned APK will be available at:
`${ROOT}/pokerth/build-android-${ANDROID_ARCH}/android-build/build/outputs/apk/release/android-build-release-unsigned.apk`

## Sign the APK

Generate keystore (first time only):
```bash
keytool -genkey -v -keystore my.keystore -keyalg RSA -keysize 2048 -validity 10000 -alias app
```

Sign the APK:
```bash
apksigner sign --ks my.keystore --ks-key-alias app ${ROOT}/pokerth/build-android-${ANDROID_ARCH}/android-build/build/outputs/apk/release/android-build-release-unsigned.apk
```

