# Windows cross-build (Docker / devcontainer)

Quick entry for `make windows-docker` and **Reopen in Container**. **Canonical detail:** [building-developer.md](../../docs/building-developer.md) (ensure, `SKIP_SYSTEM_PACKAGES`, unified `docker/Dockerfile`, `docker/windows/build/`). **Workflow overview:** [building.md](../../docs/building.md).

## Prerequisites

- Docker
- **jq** on the host for `make windows-docker` / `make windows-docker-installer` (`apt install jq`, `brew install jq`, or `make setup-windows` / `make setup-macos`)
- VS Code Dev Containers (optional)

## Quick start

- **Devcontainer:** Open the **repository root** → **Rebuild and Reopen in Container** (`.devcontainer/devcontainer.json`). Workspace `/workspaces/pokerth`. Run `make windows` or `make windows-installer` yourself — there is no `postCreateCommand`. First run may take a while while **ensure** / `setup.sh deps` installs **Qt** and **vcpkg** under `docker/windows/build/`.
- **Host:** `make windows-docker` from the repo root (same JSON via `run_devcontainer.py`). On **macOS**, `make windows` already uses this Docker path.

**Outputs:** `docker/windows/build/deploy/` (and NSIS-related artifacts under `docker/windows/` for installer targets).

## Without VS Code

From **repo root:** `make windows-docker`. Only Docker required. Image build may be skipped if the tag already exists — force rebuild: `DOCKER_FORCE_BUILD=1` or `docker rmi` (see **building-developer.md**).

## Devcontainer checklist

1. Open the **repo root**, not only `docker/windows/`.
2. **Dev Containers: Rebuild and Reopen in Container**.
3. Run `make windows` (or `make windows-installer`).
4. Confirm `pwd` is `/workspaces/pokerth` and `ls docker/windows/build/deploy` lists `pokerth_client.exe`, Qt DLLs, `data/`, `plugins/`, `qt.conf`.

**Clean rebuild:** remove `docker/windows/build` on the host (or in the container), then `make windows` again.

**Installer:** On Linux use `make windows-installer` (host MinGW) or `make windows-docker-installer`. On macOS, `make windows-installer` uses Docker.
