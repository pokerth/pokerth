# Android image: base + final: toolchain only (SDK/NDK/Gradle + .manifest.env). No RUN --mount.
# Heavy deps (Qt aqt, vcpkg) run under docker run (ensure_docker_deps.py → setup.sh deps).
# Host persistence: devcontainer binds docker/android/build/{vcpkg,Qt} onto /opt/pokerth-android/{vcpkg,Qt};
# SDK/NDK stay in the image under /opt/pokerth-android (workspace bind mount does not cover /opt).
# Intended: make android-docker (run_devcontainer.py) or devcontainer rebuild.
#
# Cache: apt runs in a layer that only depends on scripts/android-apt-packages.txt.
#
# ROOT/VCPKG_DIR/BUILD_DIR match Makefile REPO_BUILD_ROOT when using this image; setup_android.sh defaults caches. SKIP_SYSTEM_PACKAGES = skip apt in setup only.

# Stage "base": everything before setup.sh (android).
FROM ubuntu:25.10 AS base

ARG CURL_OPTS="--connect-timeout 15 --max-time 180 --retry 3 --retry-delay 5 -fSL"
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
ARG LANG=de_DE.UTF-8
ARG LANGUAGE=de_DE.UTF-8
ARG LC_ALL=de_DE.UTF-8

# So later stages (e.g. final) can USER ${USERNAME}; ARG alone is not inherited across FROM.
ENV USERNAME=${USERNAME}

ENV DEBIAN_FRONTEND=noninteractive
ENV TARGET=pokerth_client
# setup.sh dispatch; do not rely on uname in the image.
ENV TARGET_PLATFORM=android

# Architecture names: android (armeabi-v7a/arm64-v8a/x86/x86_64), vcpkg (arm-android/arm64-android/...), Qt (armv7/arm64_v8a/...)
ENV ANDROID_ARCH=arm64-v8a
ENV VCPKG_TRIPLET=arm64-android
ENV QT_ARCH=arm64_v8a

ARG ROOT=/opt/pokerth-android
ENV ROOT=${ROOT}
ENV VCPKG_DIR=${ROOT}/vcpkg
# Align with Makefile REPO_BUILD_ROOT=docker/android/build when IN_DOCKER=1.
ENV BUILD_DIR=docker/android/build
# Skip only setup_android.sh's apt-get step (same list as base). Toolchain: sdkmanager, Gradle; deps RUN: Qt paths/aqt + vcpkg.
# SKIP_SYSTEM_PACKAGES = apt/brew OS packages only — not NDK/Qt/pip/vcpkg.
ENV SKIP_SYSTEM_PACKAGES=yes

RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://de.archive.ubuntu.com/ubuntu/|' /etc/apt/sources.list.d/ubuntu.sources

# Locales first (so perl during apt does not warn)
RUN unset LANG LC_ALL LANGUAGE && apt-get update && apt-get install -y --no-install-recommends locales && rm -rf /var/lib/apt/lists/*
RUN _locale="${LANG}" && unset LANG LC_ALL LANGUAGE && sed -i '/'${_locale}'/s/^# *//' /etc/locale.gen && locale-gen
ENV LANG=${LANG}
ENV LANGUAGE=${LANGUAGE}
ENV LC_ALL=${LC_ALL}

# System packages from shared list (scripts/android-apt-packages.txt)
COPY scripts/android-apt-packages.txt /tmp/android-apt-packages.txt
RUN apt-get update && apt-get install -y --no-install-recommends \
    $(grep -v '^#' /tmp/android-apt-packages.txt | tr '\n' ' ') \
    && rm -rf /var/lib/apt/lists/*

# Docker-in-Docker (Android devcontainer convenience)
RUN install -m 0755 -d /etc/apt/keyrings && \
    curl ${CURL_OPTS} https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc && \
    chmod a+r /etc/apt/keyrings/docker.asc && \
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | \
    tee /etc/apt/sources.list.d/docker.list > /dev/null
RUN apt-get update && apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin && rm -rf /var/lib/apt/lists/*

ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# User and dir for SDK/NDK/Qt/vcpkg (setup_android.sh will populate these)
RUN userdel ubuntu 2>/dev/null || true; rm -Rf /home/ubuntu
RUN groupadd -g ${USER_GID} ${USERNAME} && useradd -s /bin/bash -m -d "/home/${USERNAME}" -u ${USER_UID} -g ${USERNAME} ${USERNAME}
RUN mkdir -p /etc/sudoers.d && echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} && chmod 0440 /etc/sudoers.d/${USERNAME}
RUN usermod -aG docker ${USERNAME}
RUN mkdir -p ${ROOT} && chown -R ${USERNAME}:${USERNAME} ${ROOT}

# Copy only what setup needs (scripts includes patches/); build context must be repo root.
COPY scripts /workspaces/pokerth/scripts
RUN chown -R ${USERNAME}:${USERNAME} /workspaces/pokerth
WORKDIR /workspaces/pokerth

# Leave default USER root here. Stage `final` switches to ${USERNAME} before RUN setup.

FROM base AS final
LABEL org.opencontainers.image.description="PokerTH Android devcontainer (image: toolchain only; deps on docker run)"

USER ${USERNAME}

# sdkmanager/Gradle write under /opt/pokerth-android. BUILD_DIR under the repo is created by setup_android.sh (same user as this RUN) so manifests/stamps are not root-owned.
RUN ./scripts/setup.sh toolchain

# setup_android.sh writes ${ROOT}/.manifest.env when ROOT=/opt/pokerth-android (survives workspace bind mount).
