# Code signing with SignPath

`.github/workflows/windows.yml` signs the Windows build through
[SignPath](https://signpath.io). The certificate and private key never leave
SignPath: the workflow uploads the unsigned files as a GitHub artifact, asks
SignPath to sign them, and downloads the signed files again.

Signing happens in two rounds:

1. `pokerth_client.exe` and `pokerth_qml-client.exe`, before `makensis`
   packs them (artifact configuration `executables`).
2. The finished `PokerTH-Combined-*-Setup.exe` (artifact configuration
   `installer`).

With `release-signing`, each round waits for a manual approval in SignPath,
so a release build needs two approvals.

Both artifact configurations only sign files whose version info says
ProductName `PokerTH`, CompanyName `PokerTH Team` and ProductVersion equal to
the `version` parameter. The workflow passes `PRODUCT_VERSION` from
`installer_combined.nsi` and fails early if it differs from `QML_VERSION_*` in
`src/game_defs.h`, which is where the exes get their version from
(`pokerth.rc.in`, `cmake/PokerthWindowsResource.cmake`). On a version bump
both have to change together.

## One-time setup in SignPath

Open-source projects can get a free certificate through the
[SignPath Foundation](https://signpath.org) (apply at signpath.org/apply). The
certificate is then issued to "SignPath Foundation", not to PokerTH, and the
project has to publish a code signing policy page.

In the SignPath organization:

1. **Trusted build system:** add the predefined *GitHub.com* trusted build
   system to the organization and install the SignPath GitHub App on the
   repository.
2. **Project:** create a project with slug `pokerth` and link the GitHub.com
   trusted build system to it.
3. **Artifact configurations:** add two, and paste the XML from this folder:
   - slug `executables` ← `executables.xml`
   - slug `installer` ← `installer.xml`
4. **Signing policies:** `test-signing` (self-signed test certificate, no
   approval) and `release-signing` (real certificate, manual approval). With
   the SignPath Foundation both already exist.
5. **API token:** create a CI user (or use your own user's API token) that is
   a *submitter* on both signing policies.

## One-time setup in GitHub

Repository → Settings → Secrets and variables → Actions:

| Kind     | Name                       | Value                                    |
|----------|----------------------------|------------------------------------------|
| Secret   | `SIGNPATH_API_TOKEN`       | API token of the CI user                 |
| Variable | `SIGNPATH_ORGANIZATION_ID` | organization ID (SignPath → Settings)    |
| Variable | `SIGNPATH_PROJECT_SLUG`    | only if the project slug is not `pokerth` |

## Running it

Actions → *Build Windows Installer* → *Run workflow*, choose the signing
policy:

- `test-signing`: signed with the test certificate. Useful to check the
  pipeline; Windows does not trust this signature. Falls back to an unsigned
  build (with a warning) if the secret is missing, e.g. in forks.
- `release-signing`: the real certificate. Fails if SignPath is not
  configured instead of producing an unsigned installer.
- `none`: no signing.

## Not signed

- The Qt and MinGW DLLs: third-party binaries.
- `Uninstall.exe`: NSIS generates it inside the installer at build time
  (`WriteUninstaller`), so there is no separate file to hand to SignPath.
