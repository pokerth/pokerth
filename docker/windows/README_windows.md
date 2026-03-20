## Prerequisites

- Docker
- **jq** (on host: for `make windows-docker` / `make docker-windows-installer`; install with `apt install jq`, `brew install jq`, or run `make setup-windows` on Linux / `make setup-macos` on macOS)
- VS Code with Dev Containers extension (ms-vscode-remote.remote-containers)

## Build Instructions

Best practice is to use the VS Code Dev Containers feature: open the **docker/windows** folder, then **Rebuild and Reopen in Container**. The workspace inside the container is the **repo root** (mounted at `/workspaces/pokerth`).

Inside the running container:

```bash
make windows
```

Or **`make windows-installer`** to build and create the NSIS installer. Output is in **docker/windows/build/deploy/** (and the installer in **docker/windows/**). The container runs **`make windows`** automatically after create (postCreateCommand).

**`docker/windows/build_windows.sh`** is **deprecated** (stderr warning only); use **`make windows`** or **`make windows-docker`**. Build logic: **scripts/build.sh** / **functions.sh**. Devcontainer / **`make windows-docker`**: **scripts/ensure_docker_deps.py windows** runs **scripts/setup.sh** when vcpkg/protobuf missing (**SKIP_SYSTEM_PACKAGES** set in Python), then **make windows**. Config knobs: **scripts/ensure_docker_deps_config.env** (e.g. Qt path template, stamp file). See **docs/building-developer.md**. Clean Docker tree: remove **docker/windows/build** on the host, then **`make windows-docker`** again.

## Testing from the command line (no VS Code/Cursor)

From the **repo root**, run:

```bash
make windows
```

On macOS **`make windows`** uses Docker (same as **`make windows-docker`**). On any host you can run:

```bash
make windows-docker
```

For the NSIS installer: on **Linux** you can use **`make windows-installer`** (host MinGW) or **`make docker-windows-installer`** (Docker). On **macOS**, **`make windows-installer`** uses Docker (no choice; same as **`make docker-windows-installer`**).

The Makefile builds the devcontainer image (if needed), runs a container with the repo mounted at `/workspaces/pokerth`, and runs **`make windows`** (or **`make windows-installer`**) inside it. Output ends up in **docker/windows/build/deploy/** (build dir is under docker/windows). Requires only Docker.

## Testing the devcontainer (VS Code/Cursor)

1. Open the **docker/windows** folder in VS Code/Cursor (File → Open Folder → choose `docker/windows`).
2. Command Palette (**Ctrl+Shift+P** / **Cmd+Shift+P**) → **Dev Containers: Rebuild and Reopen in Container**.
3. Wait for the container to start. **postCreateCommand** runs **`make windows`** automatically; watch the terminal for success or failure.
4. In the container terminal, confirm workspace is repo root and deploy exists:
   ```bash
   pwd                    # should be /workspaces/pokerth
   ls docker/windows/build/deploy # should list pokerth_client.exe, Qt DLLs, data/, plugins/, qt.conf
   ```
   If postCreateCommand failed or you want to rebuild: run **`make windows`**. For a clean build, remove **docker/windows/build** on the host (or inside the container), then run **`make windows`** again. Re-check **docker/windows/build/deploy/**.
