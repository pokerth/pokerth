# PokerTH F-Droid-Repository

PokerTH liefert den QML-Client für Android über ein **eigenes F-Droid-Repository**
aus. Nutzer fügen es einmalig in der F-Droid-App hinzu (URL oder QR-Code) und
bekommen danach Updates wie bei jeder anderen F-Droid-App.

## Warum nicht f-droid.org?

f-droid.org kompiliert jede App selbst aus dem Quellcode und akzeptiert keine
vorkompilierten Qt-Bibliotheken von `download.qt.io` — der Sinn ihrer Infrastruktur
ist gerade die Aussage „das ausgelieferte Binary entspricht diesem Quellcode",
und bei einer Qt-App wäre der größte Teil des Payloads fremder Binärcode. Erlaubt
wäre nur, Qt im F-Droid-Buildrezept **aus Quellen** zu bauen (so macht es Krita,
mit über acht Stunden Build-Timeout).

Statt Android-Nutzer ohne F-Droid-Weg zu lassen, signieren wir ein eigenes Repo —
dasselbe Modell wie KDE, Guardian Project oder Bitwarden.

## Verzeichnis

| Pfad | Inhalt |
|---|---|
| `config.yml` | Repo-Konfiguration für `fdroid update` (keine Geheimnisse) |
| `metadata/net.pokerth.PokerTH_QML.yml` | nicht-lokalisierte App-Metadaten |
| `metadata/net.pokerth.PokerTH_QML/<locale>/` | Fastlane-Texte (en-US, de) |
| `changelogs/<versionName>.txt` | optionaler Changelog pro Release |
| `site/index.html.in` | Vorlage der Landing-Page mit QR-Code |
| `repo/`, `archive/`, `keystore.p12` | Laufzeit-Artefakte, nicht in git |

Icon und Screenshots liegen bewusst nicht im git: der Workflow rendert das Icon
aus `src/gui/qt6-qml/resources/pokerth.svg` und nimmt die Tisch-Previews aus
`data/gfx/qml/table/*/preview.png`, damit die Store-Darstellung nicht von der
App abdriften kann.

## Einmalige Einrichtung

### 1. Index-Schlüssel erzeugen

Das ist **nicht** der APK-Signaturschlüssel, sondern der Schlüssel, mit dem der
Repo-Index signiert wird. Clients pinnen dessen Fingerprint.

```bash
keytool -genkeypair -v \
  -keystore fdroid-index.p12 -storetype PKCS12 \
  -alias pokerth-fdroid-index \
  -keyalg RSA -keysize 4096 -validity 10000 \
  -dname "CN=PokerTH F-Droid repo, O=PokerTH GbR, C=DE"
```

PKCS12 verlangt für Key- und Keystore-Passwort denselben Wert.

> ⚠️ **Schlüssel sichern.** Geht er verloren oder wird er ausgetauscht, muss
> *jeder* Nutzer das Repository entfernen und neu hinzufügen — es gibt keinen
> Migrationspfad. Dasselbe gilt für die Repo-URL.

```bash
base64 -w0 fdroid-index.p12 > fdroid-index.p12.b64   # Inhalt → Secret
```

### 2. Secrets und Variables in GitHub setzen

Settings → Secrets and variables → Actions.

**Secrets**

| Name | Wert |
|---|---|
| `FDROID_KEYSTORE_BASE64` | Inhalt von `fdroid-index.p12.b64` |
| `FDROID_KEYSTORE_PASS` | Keystore-Passwort |
| `FDROID_KEY_PASS` | Key-Passwort (bei PKCS12 identisch; kann entfallen) |
| `FDROID_KEY_ALIAS` | `pokerth-fdroid-index` |
| `FDROID_SSH_KEY` | privater SSH-Key mit Schreibrecht im Webroot |
| `FDROID_SSH_KNOWN_HOSTS` | optional, Ausgabe von `ssh-keyscan pokerth.net` |

**Variables**

| Name | Beispiel |
|---|---|
| `FDROID_SSH_HOST` | `pokerth.net` |
| `FDROID_SSH_USER` | `fdroid` |
| `FDROID_SSH_PATH` | `/var/www/pokerth.net/fdroid` |

Die APK-Signatur-Secrets (`ANDROID_KEYSTORE_*`) sind bereits gesetzt und werden
von `android-apk.yml` benutzt — der F-Droid-Workflow prüft nur noch, dass die
APKs tatsächlich signiert sind.

### 3. Webserver

Der Workflow legt unterhalb von `FDROID_SSH_PATH` an:

```
/var/www/pokerth.net/fdroid/
├── index.html      Landing-Page mit QR-Code
├── qr.png
├── icon.png
├── repo/           APKs + signierter Index
└── archive/        ältere Releases
```

Nötig ist nur statisches Ausliefern; Directory-Listing wird nicht gebraucht.
`repo_url` in `config.yml` muss exakt der öffentlichen URL von `repo/`
entsprechen (aktuell `https://www.pokerth.net/fdroid/repo`).

**Cloudflare:** Zwei Regeln sind hier wichtig.

1. Die bestehende User-Agent-Filterung auf pokerth.net würde die F-Droid-App
   aussperren — sie kommt mit `F-Droid/<version>`, nicht mit dem PokerTH-UA.
   Für `/fdroid/*` braucht es eine Ausnahme (Skip-Regel), sonst sehen Nutzer nur
   „Repository nicht erreichbar".
2. `.apk` wird per Default nicht gecacht. Eine Cache-Rule auf `/fdroid/repo/*`
   spart bei ~80 MB pro Datei spürbar Traffic am Origin.

## Ablauf pro Release

1. **APKs bauen** — `android-apk.yml` mit `variant: all` starten. Sie erzeugt
   pro Variante eine signierte APK mit eigenem versionCode:

   | Variante | ABI / Qt | versionCode |
   |---|---|---|
   | `qml-arm64-qt67-api26` | arm64, Qt 6.7, minSdk 26 | `<base>1` |
   | `qml-armv7` | armeabi-v7a | `<base>2` |
   | `qml-arm64` | arm64-v8a | `<base>3` |
   | `qml-x86_64` | x86_64 | `<base>4` |

   Das Schema ist Pflicht, nicht Kosmetik: ein F-Droid-Repo führt genau eine APK
   pro (Package, versionCode), und der Client installiert den **höchsten** Code,
   den das Gerät ausführen kann. Deshalb steigt die Reihenfolge von Qt-6.7-Fallback
   über 32-Bit-ARM zu arm64 und x86_64.

2. **Optional Changelog** anlegen: `fdroid/changelogs/2.1.7.txt` (Dateiname =
   versionName). Der Workflow verteilt ihn auf die versionCodes aller Varianten.

3. **Veröffentlichen** — `fdroid.yml` starten. Ohne Eingaben nimmt er den letzten
   erfolgreichen `android-apk.yml`-Lauf; mit `run_id` einen bestimmten. Beim
   ersten Mal lohnt ein Lauf mit `dry_run: true`: er baut und signiert den Index,
   fasst den Server aber nicht an und hängt das Ergebnis als Artifact an.

Der Workflow holt den aktuellen Repo-Stand vom Server, ergänzt die neuen APKs,
signiert den Index neu und lädt alles zurück — die Versionshistorie liegt also
auf dem Server, nicht im git. Fingerprint und Add-URL stehen danach in der
Job-Summary.

## Lokal testen

```bash
cd fdroid
export FDROID_KEY_ALIAS=pokerth-fdroid-index FDROID_KEYSTORE_PASS=… FDROID_KEY_PASS=…
cp /pfad/fdroid-index.p12 keystore.p12
mkdir -p repo && cp ../PokerTH-*.apk repo/
fdroid update --verbose --pretty
```

## Widget-Client ergänzen

Der klassische Widget-Client (`org.pokerth.widget`) ist absichtlich nicht im
Repo. Um ihn aufzunehmen: `ARTIFACT_PATTERN` in `.github/workflows/fdroid.yml`
auf `PokerTH-apk-*` erweitern und `metadata/org.pokerth.widget.yml` samt
Fastlane-Baum anlegen.
