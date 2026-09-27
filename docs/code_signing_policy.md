# Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io),
certificate by [SignPath Foundation](https://signpath.org).

## What is signed

Only binaries built from this repository by the project's GitHub Actions
workflow ([.github/workflows/windows.yml](../.github/workflows/windows.yml)) are
signed:

- the Windows clients `pokerth_client.exe` and `pokerth_qml-client.exe`
- the Windows installer `PokerTH-Combined-<version>-Setup.exe`

Third-party libraries shipped with the installer (Qt, the MinGW runtime,
OpenSSL, Boost, Protobuf) are upstream open source binaries and are not signed
by the project.

The signing request is submitted from the workflow run itself, so SignPath can
verify that the signed files were built from this repository on GitHub-hosted
runners. The configuration is in [docker/windows/signpath](../docker/windows/signpath).

## Team roles

| Role                     | Members                                                                                               |
|--------------------------|-------------------------------------------------------------------------------------------------------|
| Committers and reviewers | [Kai Philipp (@q4z1)](https://github.com/q4z1), [Hains van den Bosch (@Hains)](https://github.com/Hains) |
| Approvers                | [Kai Philipp (@q4z1)](https://github.com/q4z1)                                                        |

Committers can push to this repository; changes from outside contributors are
reviewed before they are merged. Every signing request for a release has to be
approved by an approver in SignPath before a release certificate is used.

All team members use multi-factor authentication for GitHub and SignPath.

## Privacy policy

This program will not transfer any information to other networked systems
unless specifically requested by the user or the person installing or
operating it.

Network connections are made only for features the user starts: playing on
an internet or LAN server, opening links to the website, uploading a log file
for analysis, or translating a chat message (see
[third_party_services.md](third_party_services.md)). The privacy policy covering
the official game server and website is published at
[www.pokerth.net/ucp.php?mode=privacy](https://www.pokerth.net/ucp.php?mode=privacy).
