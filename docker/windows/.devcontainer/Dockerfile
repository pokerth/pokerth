# Windows cross image: base + final RUN ./scripts/setup.sh toolchain as vscode. No RUN --mount.
# Toolchain (MinGW, etc.) in image; Qt + vcpkg under docker run (ensure_docker_deps → setup.sh deps).
# Host persistence: devcontainer binds docker/windows/build/{vcpkg,Qt} onto /opt/pokerth-windows/{vcpkg,Qt}.
# Intended: make windows-docker or devcontainer rebuild.
#
# Cache: apt runs in a layer that only depends on scripts/windows-apt-packages.txt; other script/doc changes don't invalidate it. setup.sh skips apt via ENV SKIP_SYSTEM_PACKAGES=yes (base stage).

# Stage "base": OS + deps + vscode; scripts (incl. patches/) for in-container setup and build.
FROM ubuntu:25.10 AS base

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
ARG LANG=de_DE.UTF-8
ARG LANGUAGE=de_DE.UTF-8
ARG LC_ALL=de_DE.UTF-8

ENV DEBIAN_FRONTEND=noninteractive
ENV TARGET=pokerth_client
# Skip only setup_linux.sh's apt step for windows-apt-packages.txt (base already ran it). vcpkg/Qt/nsis/etc. still run per setup.
# SKIP_SYSTEM_PACKAGES = apt OS packages only.
ENV SKIP_SYSTEM_PACKAGES=yes

RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://de.archive.ubuntu.com/ubuntu/|' /etc/apt/sources.list.d/ubuntu.sources

# Locales first so perl (during apt) does not warn
RUN unset LANG LC_ALL LANGUAGE && apt-get update && apt-get install -y --no-install-recommends locales && rm -rf /var/lib/apt/lists/*
RUN _locale="${LANG}" && unset LANG LC_ALL LANGUAGE && sed -i '/'${_locale}'/s/^# *//' /etc/locale.gen && locale-gen
ENV LANG=${LANG}
ENV LANGUAGE=${LANGUAGE}
ENV LC_ALL=${LC_ALL}

# System packages from shared list (scripts/windows-apt-packages.txt); only this COPY + RUN invalidate when the list changes.
COPY scripts/windows-apt-packages.txt /tmp/windows-apt-packages.txt
RUN apt-get update && apt-get install -y --no-install-recommends \
    $(grep -v '^#' /tmp/windows-apt-packages.txt | tr '\n' ' ') \
    && rm -rf /var/lib/apt/lists/*

# abseil (and other C++11 code) needs std::mutex; MinGW win32 thread model does not provide it. Use posix variant for vcpkg x64-mingw-static.
RUN for t in gcc g++; do update-alternatives --set x86_64-w64-mingw32-${t} /usr/bin/x86_64-w64-mingw32-${t}-posix 2>/dev/null || true; done

# User and dir for vcpkg/Qt (setup.sh will populate these)
RUN userdel ubuntu 2>/dev/null || true; rm -Rf /home/ubuntu
RUN groupadd -g ${USER_GID} ${USERNAME} && useradd -s /bin/bash -m -d /home/vscode -u ${USER_UID} -g ${USERNAME} ${USERNAME}
RUN mkdir -p /etc/sudoers.d && echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} && chmod 0440 /etc/sudoers.d/${USERNAME}
RUN mkdir -p /opt/pokerth-windows && chown -R ${USERNAME}:${USERNAME} /opt/pokerth-windows

# Copy only what setup.sh needs (scripts includes patches/); build context must be repo root.
# COPY creates root-owned files; chown so vscode can write (e.g. aqtinstall.log during setup).
COPY scripts /workspaces/pokerth/scripts
RUN chown -R ${USERNAME}:${USERNAME} /workspaces/pokerth
WORKDIR /workspaces/pokerth

# Default to vscode so make/setup create repo and cache files as vscode (not root); matches remoteUser in devcontainer.
USER ${USERNAME}

FROM base AS final
LABEL org.opencontainers.image.description="PokerTH Windows cross devcontainer (toolchain in image; deps on docker run)"

# Toolchain only (MinGW + OS packages already in base). Qt + vcpkg run under docker run (ensure_docker_deps → setup.sh deps).
RUN mkdir -p /opt/pokerth-windows \
    && VCPKG_DIR=/opt/pokerth-windows/vcpkg \
       QT_OUTPUT_DIR=/opt/pokerth-windows/Qt \
       TARGET_PLATFORM=windows \
       USE_AQT=yes \
       USE_VCPKG=yes \
       ./scripts/setup.sh toolchain

# At runtime the repo is mounted at /workspaces/pokerth; make windows runs scripts/build.sh.
