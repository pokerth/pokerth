# F-Droid — state and open points

As of 09.08.2026. This document records why the F-Droid path looks the way it
does, what is finished in the repo and what is still missing to carry on. The
step-by-step setup is in [`fdroid/README.md`](../../fdroid/README.md); this one
is about the decision and the current state.

## Starting point: f-droid.org does not work for us

f-droid.org compiles **every** app from source on its own build servers — that
is the whole point of it: "The F-Droid infrastructure compiles applications from
publicly accessible source code to verify that distributed binaries match their
source code."

For Qt apps that means:

* Precompiled Qt from `download.qt.io` (aqtinstall, `install-qt.sh`) is **not**
  allowed. The inclusion policy does have an exception allowlist for prebuilts —
  Debian main, Maven Central/Google Maven/Sonatype/JFrog/JitPack, Android SDK,
  Flutter SDK, PyPI wheels, Rust/Go/Node — but Qt is not on it. The difference is
  systematic: those are build tools that are not shipped along. The Qt libs, on
  the other hand, are the largest part of the payload in our APK; F-Droid's core
  statement would be worthless with them.
* The maintainers' position on it: "Ideally, we would like to build all
  dependencies from the source code to make sure that no proprietary bits
  sneaked into binaries" and "We value freedom more than compilation speed."
* The only thing that would be allowed is **building Qt from source inside the
  build recipe**. The precedent is Krita (`metadata/org.krita.yml` in
  fdroiddata): the stages `boost` → `qt` → `3rdparty` → `kf5` → app, with
  `timeout: 30000` (more than 8 hours). For PokerTH that would mean
  qtbase/qtdeclarative/qtmultimedia/qtsvg + boost/openssl/protobuf from source,
  per ABI, plus permanent maintenance with every Qt update — and Krita does not
  even use `androiddeployqt`, for which there is no established recipe.
* The "reproducible builds" path (we sign, F-Droid only verifies) does not help
  either: F-Droid still has to be able to reproduce the build.

Also examined and rejected: **IzzyOnDroid** (the usual third-party repo that
pulls APKs straight from GitHub releases). Its inclusion policy takes no games,
and the size limit is around 30 MB per APK — the QML client is far above that
with 94 MB of `data/` alone.

## Decision: our own repo

A self-signed F-Droid repository, hosted on the PokerTH web server — the same
model as KDE, the Guardian Project or Bitwarden. Users add it once by URL or QR
code and get updates afterwards like with any other F-Droid app.

## What is in the repo

| File | Purpose |
|---|---|
| [`.github/workflows/fdroid.yml`](../../.github/workflows/fdroid.yml) | the publishing workflow (builds nothing, only publishes) |
| [`fdroid/config.yml`](../../fdroid/config.yml) | repo configuration for `fdroid update`, no secrets |
| [`fdroid/metadata/`](../../fdroid/metadata/) | app metadata + fastlane texts (en-US, de) |
| [`fdroid/site/index.html.in`](../../fdroid/site/index.html.in) | template of the landing page with the QR code |
| [`fdroid/README.md`](../../fdroid/README.md) | setup: keys, secrets, web server, release procedure |

The workflow takes the signed APK artifacts of an `android-apk.yml` run (without
an input: the last successful one), checks the signature and the versionCodes,
fetches the existing repo state from the server via rsync, adds the new APKs,
signs the index, renders the landing page and the QR code and uploads
everything back. It runs for a few minutes. The icon (from `pokerth.svg`,
512x512) and the screenshots (table previews from
`data/gfx/qml/table/*/preview.png`) are produced from the tree at runtime, so
that the presentation cannot drift away from the app.

The version history lives on the server, not in git — hence the pull before the
index is built.

## Existing code that was already changed

Since this work, [`android-apk.yml`](../../.github/workflows/android-apk.yml)
assigns a **separate versionCode** per variant: `base * 10 + ABI code`, the
scheme Play uses for ABI splits as well.

| Variant | ABI / Qt | Code |
|---|---|---|
| `qml-arm64-qt67-api26` | arm64, Qt 6.7, minSdk 26 | `<base>1` |
| `qml-armv7` | armeabi-v7a | `<base>2` |
| `qml-arm64` | arm64-v8a | `<base>3` |
| `qml-x86_64` | x86_64 | `<base>4` |

That is a prerequisite, not polish: an F-Droid repo carries exactly one APK per
(package, versionCode), and the client installs the highest code the device can
run — hence the ascending order from the Qt 6.7 fallback over 32 bit ARM to
arm64 and x86_64. Without the scheme three of the four variants would disappear
from the index.

Side effect for sideload APKs: the codes jump from ~1xx to ~1xxx. Since they
keep rising monotonically, that is harmless for existing installations.

**This change takes effect even without F-Droid** — it is the only part that
already affects every APK build today. Whoever wants to revert the whole topic
has to revert it as well; necessary it is not.

Untouched: `android.yml` (the Play `.aab`) and the local Docker build scripts.

## What is still missing

None of this is started — these are the steps when picking the topic up again:

1. **Create and back up the index keystore** (`keytool`, PKCS12, see
   `fdroid/README.md`). Clients pin its fingerprint; swapping the key forces
   every user to remove the repo and add it again.
2. **Set the secrets and variables in GitHub**: `FDROID_KEYSTORE_BASE64`,
   `FDROID_KEYSTORE_PASS`, `FDROID_KEY_PASS`, `FDROID_KEY_ALIAS`,
   `FDROID_SSH_KEY` as well as `FDROID_SSH_HOST/_USER/_PATH`.
3. **Settle `repo_url` in `fdroid/config.yml`** — it is set to
   `https://www.pokerth.net/fdroid/repo` and has to match the deploy path. The
   URL is effectively immutable afterwards, because users store it.
4. **Prepare the web server**: serve `<webroot>/fdroid/{repo,archive}`
   statically, with the landing page next to it.
5. **Cloudflare** — two rules that bite otherwise:
   * The F-Droid app comes with the user agent `F-Droid/<version>`. The existing
     UA filtering on pokerth.net would lock it out; `/fdroid/*` needs a skip
     rule, otherwise users only see "repository unreachable".
   * `.apk` is not cached by default. A cache rule on `/fdroid/repo/*` saves a
     noticeable amount of origin traffic at ~80 MB per file.
6. **First run with `dry_run: true`.** The workflow has been simulated locally
   and validated as YAML, but end to end it is naturally only tested once it has
   run against the real secrets and the real server. The dry run signs the index
   and attaches it as an artifact without touching the server.
7. **Announce it**: a link/QR code from pokerth.net to the landing page.

## Deliberately left open

* **The Widgets client** (`org.pokerth.widget`) is not in the repo. Including it
  would work via `ARTIFACT_PATTERN` in `fdroid.yml` (widened to
  `PokerTH-apk-*`) plus `fdroid/metadata/org.pokerth.widget.yml` together with
  its fastlane tree.
* **No automatic trigger.** `fdroid.yml` only runs via `workflow_dispatch`;
  coupling it to tags or to the completion of `android-apk.yml` would be
  possible, but for a repo whose index one only wants to rewrite deliberately it
  is left out on purpose.
* **Changelogs** are prepared but empty: one file
  `fdroid/changelogs/<versionName>.txt` per release is enough, the workflow
  spreads it over the versionCodes of all variants.
