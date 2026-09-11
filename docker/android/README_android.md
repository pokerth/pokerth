# Android builds

Release builds run on **GitHub Actions**, not on a local machine:

| Workflow | Result |
| --- | --- |
| [`android-apk.yml`](../../.github/workflows/android-apk.yml) | one directly installable APK per variant (client, ABI, Qt version); [`fdroid.yml`](../../.github/workflows/fdroid.yml) publishes the QML ones in the F-Droid repo |
| [`android.yml`](../../.github/workflows/android.yml) | the Play App Bundle (`.aab`) with every ABI at once |

Both are started manually under GitHub → Actions → *Run workflow*; the signing
secrets are listed further down.

For a quick test build on your own machine there are two local paths, neither
of which is used for releases:

- from the terminal, without entering the container:
  [README_android_docker.md](README_android_docker.md)
  (`bash docker/android/build_android_qml_arm64_docker.sh`)
- inside the VS Code dev container (Docker + the Dev Containers extension):
  set architecture and target in the Dockerfile in the `.devcontainer` folder
  (`arm64-v8a`, `armeabi-v7a`, `x86_64`; `docker-compose.yml` carries the
  network settings), then

  ```bash
  cd ${ROOT}/pokerth
  bash docker/android/build_android.sh
  ```

  The unsigned APK ends up at
  `${ROOT}/pokerth/build-android-${ANDROID_ARCH}/android-build/build/outputs/apk/release/android-build-release-unsigned.apk`

## Sign a locally built APK

Generate keystore (first time only):
```bash
keytool -genkey -v -keystore my.keystore -keyalg RSA -keysize 2048 -validity 10000 -alias app
```

Sign the APK:
```bash
apksigner sign --ks my.keystore --ks-key-alias app ${ROOT}/pokerth/build-android-${ANDROID_ARCH}/android-build/build/outputs/apk/release/android-build-release-unsigned.apk
```

## Google Play release (App Bundle)

`build_android.sh` builds one APK for one ABI — good for testing, but Google Play
only accepts an **App Bundle** (`.aab`) that carries every ABI at once. That is
what `build_android_qml.sh` (repo root) and the `Build Android App Bundle (QML
client)` workflow produce: they build each ABI separately (each one needs its own
Qt kit and vcpkg triplet), collect all of them into a single Gradle project and
run `bundleRelease` over the lot.

Run it locally exactly like CI does:

```bash
# all four ABIs, signed, plus a universal APK to side-load for testing
ANDROID_KEYSTORE=$PWD/upload.keystore ANDROID_KEYSTORE_PASS=secret \
ANDROID_KEY_ALIAS=upload ANDROID_KEY_PASS=secret \
UNIVERSAL_APK=1 VERSION_CODE=24 bash build_android_qml.sh
```

See the script header for all environment overrides (`ANDROID_ABIS`,
`ANDROID_API_LEVEL`, `VERSION_CODE`, …).

### One-time setup for the store

1. **Upload key** — keep this file forever; Play ties the app to it:
   ```bash
   keytool -genkeypair -v -keystore upload.keystore -alias upload \
     -keyalg RSA -keysize 2048 -validity 10000
   ```
   Opt into *Play App Signing*, then this is only the upload key: if it is ever
   lost, Google can reset it — the app signing key itself stays with Google.

2. **GitHub secrets** for the workflow (Settings → Secrets → Actions):
   | Secret | Value |
   | --- | --- |
   | `ANDROID_KEYSTORE_BASE64` | `base64 -w0 upload.keystore` |
   | `ANDROID_KEYSTORE_PASSWORD` | keystore password |
   | `ANDROID_KEY_ALIAS` | `upload` |
   | `ANDROID_KEY_PASSWORD` | key password |
   | `GOOGLE_PLAY_SERVICE_ACCOUNT_JSON` | only needed to publish automatically |

   Without them the workflow still runs and produces an **unsigned** bundle,
   which Play rejects — it is only good for inspection.

3. **First upload must be manual.** The Play API cannot create an app, so upload
   the first `.aab` by hand in the Play Console (Play Console → create app →
   internal testing). Only afterwards can `play_track` publish from CI.

### Rules the bundle has to satisfy

- `versionCode` must be **higher than every previous upload** — Play rejects
  duplicates. The workflow defaults to the run number; override it with the
  `version_code` input.
- `targetSdk`: Play requires **API 36 for new apps and updates from
  2026-08-31**. Use the `api_level` input to switch from 35 to 36.
- `minSdk` is 28 (Android 9) — the floor Qt 6.8+ supports, so it cannot go lower.
- ABIs: `arm64-v8a` covers every current phone, `x86_64` adds Chromebooks and
  emulators, `armeabi-v7a` old 32-bit devices, `x86` is effectively
  emulator-only. Play serves each device only the slice it needs, so extra ABIs
  cost build time, not download size.

