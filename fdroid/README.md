# PokerTH F-Droid repository

PokerTH ships the QML client for Android through an **F-Droid repository of its
own**. Users add it to the F-Droid app once (by URL or QR code) and get updates
afterwards like with any other F-Droid app.

## Why not f-droid.org?

f-droid.org compiles every app from source itself and accepts no precompiled Qt
libraries from `download.qt.io` — the very point of their infrastructure is the
statement "the shipped binary corresponds to this source code", and with a Qt
app the largest part of the payload would be foreign binary code. The only thing
that would be allowed is building Qt **from source** inside the F-Droid build
recipe (the way Krita does it, with a build timeout of more than eight hours).

Rather than leaving Android users without an F-Droid path, we sign a repository
of our own — the same model as KDE, the Guardian Project or Bitwarden.

## Directory

| Path | Contents |
|---|---|
| `config.yml` | repo configuration for `fdroid update` (no secrets) |
| `metadata/net.pokerth.PokerTH_QML.yml` | non-localized app metadata |
| `metadata/net.pokerth.PokerTH_QML/<locale>/` | fastlane texts (en-US, de) |
| `changelogs/<versionName>.txt` | optional changelog per release |
| `site/index.html.in` | template of the landing page with the QR code |
| `repo/`, `archive/`, `keystore.p12` | runtime artifacts, not in git |

The icon and the screenshots are deliberately not in git: the workflow renders
the icon from `src/gui/qt6-qml/resources/pokerth.svg` and takes the table
previews from `data/gfx/qml/table/*/preview.png`, so that the store presentation
cannot drift away from the app.

## One-time setup

### 1. Create the index key

This is **not** the APK signing key, but the key the repo index is signed with.
Clients pin its fingerprint.

```bash
keytool -genkeypair -v \
  -keystore fdroid-index.p12 -storetype PKCS12 \
  -alias pokerth-fdroid-index \
  -keyalg RSA -keysize 4096 -validity 10000 \
  -dname "CN=PokerTH F-Droid repo, O=PokerTH GbR, C=DE"
```

PKCS12 requires the same value for the key and the keystore password.

> ⚠️ **Back the key up.** If it is lost or swapped, *every* user has to remove
> the repository and add it again — there is no migration path. The same goes
> for the repo URL.

```bash
base64 -w0 fdroid-index.p12 > fdroid-index.p12.b64   # content → secret
```

### 2. Set the secrets and variables in GitHub

Settings → Secrets and variables → Actions.

**Secrets**

| Name | Value |
|---|---|
| `FDROID_KEYSTORE_BASE64` | content of `fdroid-index.p12.b64` |
| `FDROID_KEYSTORE_PASS` | keystore password |
| `FDROID_KEY_PASS` | key password (identical with PKCS12; may be omitted) |
| `FDROID_KEY_ALIAS` | `pokerth-fdroid-index` |
| `FDROID_SSH_KEY` | private SSH key with write access in the web root |
| `FDROID_SSH_KNOWN_HOSTS` | optional, the output of `ssh-keyscan pokerth.net` |

**Variables**

| Name | Example |
|---|---|
| `FDROID_SSH_HOST` | `pokerth.net` |
| `FDROID_SSH_USER` | `fdroid` |
| `FDROID_SSH_PATH` | `/var/www/pokerth.net/fdroid` |

The APK signing secrets (`ANDROID_KEYSTORE_*`) are already set and are used by
`android-apk.yml` — the F-Droid workflow only checks that the APKs really are
signed.

### 3. Web server

Below `FDROID_SSH_PATH` the workflow creates:

```
/var/www/pokerth.net/fdroid/
├── index.html      landing page with the QR code
├── qr.png
├── icon.png
├── repo/           APKs + signed index
└── archive/        older releases
```

All that is needed is static serving; a directory listing is not required.
`repo_url` in `config.yml` has to match the public URL of `repo/` exactly
(currently `https://www.pokerth.net/fdroid/repo`).

**Cloudflare:** two rules matter here.

1. The existing user agent filtering on pokerth.net would lock the F-Droid app
   out — it comes with `F-Droid/<version>`, not with the PokerTH UA. `/fdroid/*`
   needs an exception (a skip rule), otherwise users only see "repository
   unreachable".
2. `.apk` is not cached by default. A cache rule on `/fdroid/repo/*` saves a
   noticeable amount of origin traffic at ~80 MB per file.

## Procedure per release

1. **Build the APKs** — start `android-apk.yml` with `variant: all`. It produces
   one signed APK per variant, each with its own versionCode:

   | Variant | ABI / Qt | versionCode |
   |---|---|---|
   | `qml-arm64-qt67-api26` | arm64, Qt 6.7, minSdk 26 | `<base>1` |
   | `qml-armv7` | armeabi-v7a | `<base>2` |
   | `qml-arm64` | arm64-v8a | `<base>3` |
   | `qml-x86_64` | x86_64 | `<base>4` |

   The scheme is mandatory, not cosmetics: an F-Droid repo carries exactly one
   APK per (package, versionCode), and the client installs the **highest** code
   the device can run. That is why the order rises from the Qt 6.7 fallback over
   32 bit ARM to arm64 and x86_64.

2. **Optionally add a changelog**: `fdroid/changelogs/2.1.8.txt` (the file name
   is the versionName). The workflow spreads it over the versionCodes of all
   variants.

3. **Publish** — start `fdroid.yml`. Without any input it takes the last
   successful `android-apk.yml` run; with `run_id` a specific one. The first
   time, a run with `dry_run: true` is worth it: it builds and signs the index
   but does not touch the server and attaches the result as an artifact.

The workflow fetches the current repo state from the server, adds the new APKs,
signs the index anew and uploads everything back — so the version history lives
on the server, not in git. The fingerprint and the add URL are in the job
summary afterwards.

## Testing locally

```bash
cd fdroid
export FDROID_KEY_ALIAS=pokerth-fdroid-index FDROID_KEYSTORE_PASS=… FDROID_KEY_PASS=…
cp /path/fdroid-index.p12 keystore.p12
mkdir -p repo && cp ../PokerTH-*.apk repo/
fdroid update --verbose --pretty
```

## Adding the Widgets client

The classic Widgets client (`org.pokerth.widget`) is deliberately not in the
repo. To include it: widen `ARTIFACT_PATTERN` in
`.github/workflows/fdroid.yml` to `PokerTH-apk-*` and create
`metadata/org.pokerth.widget.yml` together with its fastlane tree.
