# F-Droid — Stand und offene Punkte

Stand: 09.08.2026. Dieses Dokument hält fest, warum der F-Droid-Weg so aussieht,
wie er aussieht, was fertig im Repo liegt und was zum Weitermachen noch fehlt.
Die Schritt-für-Schritt-Einrichtung steht in [`fdroid/README.md`](../../fdroid/README.md);
hier geht es um Entscheidung und Zustand.

## Ausgangslage: f-droid.org geht nicht

f-droid.org kompiliert **jede** App selbst aus dem Quellcode auf eigenen
Buildservern — das ist der Sinn des Ganzen: „The F-Droid infrastructure compiles
applications from publicly accessible source code to verify that distributed
binaries match their source code."

Für Qt-Apps bedeutet das:

* Vorkompiliertes Qt von `download.qt.io` (aqtinstall, `install-qt.sh`) ist
  **nicht** erlaubt. Die Inclusion Policy hat zwar eine Ausnahme-Allowlist für
  Prebuilts — Debian main, Maven Central/Google Maven/Sonatype/JFrog/JitPack,
  Android SDK, Flutter SDK, PyPI-Wheels, Rust/Go/Node — aber Qt steht nicht
  darauf. Der Unterschied ist systematisch: das sind Build-Werkzeuge, die nicht
  mit ausgeliefert werden. Qt-Libs sind dagegen der größte Teil des Payloads in
  unserer APK; F-Droids Kernaussage wäre damit wertlos.
* Maintainer-Position dazu: „Ideally, we would like to build all dependencies
  from the source code to make sure that no proprietary bits sneaked into
  binaries" und „We value freedom more than compilation speed."
* Erlaubt wäre nur, **Qt im Buildrezept aus Quellen zu bauen**. Präzedenzfall ist
  Krita (`metadata/org.krita.yml` in fdroiddata): Stufen `boost` → `qt` →
  `3rdparty` → `kf5` → App, `timeout: 30000` (über 8 Stunden). Für PokerTH hieße
  das qtbase/qtdeclarative/qtmultimedia/qtsvg + boost/openssl/protobuf aus
  Quellen, pro ABI, plus dauerhafte Pflege bei jedem Qt-Update — und Krita nutzt
  nicht einmal `androiddeployqt`, für das es kein etabliertes Rezept gibt.
* Auch der „Reproducible Builds"-Weg (wir signieren, F-Droid verifiziert nur)
  hilft nicht: F-Droid muss den Build trotzdem nachvollziehen können.

Geprüft und ebenfalls verworfen: **IzzyOnDroid** (das übliche Drittanbieter-Repo,
das APKs direkt aus GitHub-Releases zieht). Die Inclusion Policy nimmt keine
Spiele, und das Größenlimit liegt bei rund 30 MB pro APK — der QML-Client liegt
mit allein 94 MB `data/` weit darüber.

## Entscheidung: eigenes Repo

Ein selbst signiertes F-Droid-Repository, gehostet auf dem PokerTH-Webserver —
dasselbe Modell wie KDE, Guardian Project oder Bitwarden. Nutzer fügen es einmal
per URL oder QR-Code hinzu und bekommen danach Updates wie bei jeder anderen
F-Droid-App.

## Was im Repo liegt

| Datei | Zweck |
|---|---|
| [`.github/workflows/fdroid.yml`](../../.github/workflows/fdroid.yml) | Veröffentlichungs-Workflow (baut nichts, publiziert nur) |
| [`fdroid/config.yml`](../../fdroid/config.yml) | Repo-Konfiguration für `fdroid update`, keine Geheimnisse |
| [`fdroid/metadata/`](../../fdroid/metadata/) | App-Metadaten + Fastlane-Texte (en-US, de) |
| [`fdroid/site/index.html.in`](../../fdroid/site/index.html.in) | Vorlage der Landing-Page mit QR-Code |
| [`fdroid/README.md`](../../fdroid/README.md) | Einrichtung: Schlüssel, Secrets, Webserver, Release-Ablauf |

Der Workflow zieht die signierten APK-Artifacts eines `android-apk.yml`-Laufs
(ohne Eingabe: der letzte erfolgreiche), prüft Signatur und versionCodes, holt
den bestehenden Repo-Stand per rsync vom Server, ergänzt die neuen APKs,
signiert den Index, rendert Landing-Page und QR-Code und lädt alles zurück.
Laufzeit einige Minuten. Icon (aus `pokerth.svg`, 512×512) und Screenshots
(Tisch-Previews aus `data/gfx/qml/table/*/preview.png`) entstehen zur Laufzeit
aus dem Tree, damit die Darstellung nicht von der App abdriften kann.

Die Versionshistorie liegt auf dem Server, nicht im git — deshalb der Pull vor
dem Index-Bau.

## Bereits geänderter Bestandscode

[`android-apk.yml`](../../.github/workflows/android-apk.yml) vergibt seit dieser
Arbeit pro Variante einen **eigenen versionCode**: `base * 10 + ABI-Code`, das
Schema, das auch Play für ABI-Splits nutzt.

| Variante | ABI / Qt | Code |
|---|---|---|
| `qml-arm64-qt67-api26` | arm64, Qt 6.7, minSdk 26 | `<base>1` |
| `qml-armv7` | armeabi-v7a | `<base>2` |
| `qml-arm64` | arm64-v8a | `<base>3` |
| `qml-x86_64` | x86_64 | `<base>4` |

Das ist Voraussetzung, kein Feinschliff: ein F-Droid-Repo führt genau eine APK
pro (Package, versionCode), und der Client installiert den höchsten Code, den
das Gerät ausführen kann — daher die aufsteigende Reihenfolge vom
Qt-6.7-Fallback über 32-Bit-ARM zu arm64 und x86_64. Ohne das Schema würden drei
der vier Varianten im Index verschwinden.

Nebeneffekt für Sideload-APKs: die Codes springen von ~1xx auf ~1xxx. Weil sie
monoton weiter steigen, ist das für bestehende Installationen unkritisch.

**Diese Änderung wirkt auch ohne F-Droid** — sie ist der einzige Teil, der schon
jetzt jeden APK-Build betrifft. Wer das Thema ganz zurückdrehen will, muss sie
mit zurückdrehen; nötig ist das aber nicht.

Nicht angefasst: `android.yml` (Play-.aab) und die lokalen Docker-Buildskripte.

## Was noch fehlt

Nichts davon ist angefangen — das sind die Schritte beim Wiederaufnehmen:

1. **Index-Keystore erzeugen und sichern** (`keytool`, PKCS12, siehe
   `fdroid/README.md`). Clients pinnen den Fingerprint; ein Schlüsseltausch
   zwingt jeden Nutzer, das Repo zu entfernen und neu hinzuzufügen.
2. **Secrets und Variables in GitHub setzen**: `FDROID_KEYSTORE_BASE64`,
   `FDROID_KEYSTORE_PASS`, `FDROID_KEY_PASS`, `FDROID_KEY_ALIAS`,
   `FDROID_SSH_KEY` sowie `FDROID_SSH_HOST/_USER/_PATH`.
3. **`repo_url` in `fdroid/config.yml` festlegen** — steht auf
   `https://www.pokerth.net/fdroid/repo` und muss zum Deploy-Pfad passen. Die URL
   ist danach faktisch unveränderlich, weil Nutzer sie speichern.
4. **Webserver vorbereiten**: statisches Ausliefern von
   `<webroot>/fdroid/{repo,archive}` plus Landing-Page daneben.
5. **Cloudflare** — zwei Regeln, die sonst kalt erwischen:
   * Die F-Droid-App kommt mit User-Agent `F-Droid/<version>`. Die bestehende
     UA-Filterung auf pokerth.net würde sie aussperren; `/fdroid/*` braucht eine
     Skip-Regel, sonst sehen Nutzer nur „Repository nicht erreichbar".
   * `.apk` wird per Default nicht gecacht. Eine Cache-Rule auf
     `/fdroid/repo/*` spart bei ~80 MB pro Datei spürbar Origin-Traffic.
6. **Erster Lauf mit `dry_run: true`.** Der Workflow ist lokal simuliert und
   YAML-validiert, aber end-to-end naturgemäß erst getestet, wenn er einmal
   gegen echte Secrets und den echten Server gelaufen ist. Der Dry Run signiert
   den Index und hängt ihn als Artifact an, ohne den Server anzufassen.
7. **Bekanntmachen**: Link/QR von pokerth.net auf die Landing-Page.

## Bewusst offen gelassen

* **Widget-Client** (`org.pokerth.widget`) ist nicht im Repo. Aufnehmen ginge
  über `ARTIFACT_PATTERN` in `fdroid.yml` (auf `PokerTH-apk-*` erweitern) plus
  `fdroid/metadata/org.pokerth.widget.yml` samt Fastlane-Baum.
* **Kein automatischer Trigger.** `fdroid.yml` läuft nur per
  `workflow_dispatch`; eine Kopplung an Tags oder an den Abschluss von
  `android-apk.yml` wäre möglich, ist aber bei einem Repo, dessen Index man nur
  bewusst neu schreiben will, absichtlich nicht eingebaut.
* **Changelogs** sind vorbereitet, aber leer: eine Datei
  `fdroid/changelogs/<versionName>.txt` pro Release genügt, der Workflow verteilt
  sie auf die versionCodes aller Varianten.
