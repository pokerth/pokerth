## Prerequisites

- Docker
- VS Code with Dev Containers extension (ms-vscode-remote.remote-containers)

## Build Instructions

Best practice is to use the VS Code Dev-Container feature.

Inside the running container:

```bash
cd ${ROOT}/pokerth
bash docker/windows/build_windows.sh
```

The build creates a Windows installer package (.exe) with all dependencies included.
Output location and packaging details are defined in build_windows.sh script.


