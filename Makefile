# Top-level build entry point for PokerTH (plan section 7).
# Default: make = make linux (on Linux) or make macos (on macOS). make setup = setup for current OS.
# Use: make linux, make windows, make macos, make setup-linux, make setup-windows, make setup-macos, make clean.
# Installers: make linux-installer, make windows-installer, make macos-installer, make installers.
# Pass env: CLEAN=yes make linux, BUILD_TARGET=pokerth_client make linux.

.PHONY: all
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  TARGET_PLATFORM ?= macos
else
  TARGET_PLATFORM ?= linux
endif

# Setup completion marker (basename must match MANIFEST_ENV default in scripts/functions.sh).
MANIFEST_NAME := .manifest.env

# Default target: Native builds: host toolchain.
all: build_$(TARGET_PLATFORM)/$(MANIFEST_NAME)
	@echo "Building for $(TARGET_PLATFORM)"
	@TARGET_PLATFORM=$(TARGET_PLATFORM) REPO_BUILD_ROOT=$(REPO_BUILD_ROOT) $(SCRIPTS)/build.sh

SCRIPTS := ./scripts

# Snapshot `IN_DEVCONTAINER` from the environment before redefining it (`.devcontainer` may set `IN_DEVCONTAINER=1`).
_DEVCONTAINER_ENV := $(IN_DEVCONTAINER)
# 1 when in a Dev Containers editor session: env from devcontainer.json and/or `REMOTE_CONTAINERS` (VS Code/Cursor). Not set in `docker run` from `make *-docker` on the host.
IN_DEVCONTAINER := $(if $(filter 1,$(_DEVCONTAINER_ENV)),1,$(if $(strip $(REMOTE_CONTAINERS)),1,))

# Per-target build trees (manifest + CMake + outputs):
# - host: build_linux/, build_windows/, build_macos/, build_android/
# - docker: docker/windows/build/, docker/android/build/
#
# `docker run` from `make *-docker` and Dockerfiles set `ENV IN_DOCKER=1` (devcontainer.json may add other keys).
# `IN_DEVCONTAINER=1` comes from `.devcontainer/*/devcontainer.json` `containerEnv` (editor session; not set for plain `docker run`).
NATIVE_PLATFORMS   := linux windows macos
# Docker image kinds: single ordered list — used for clean paths and for setup-toolchains (android then windows).
DOCKER_KINDS := android windows

# Dirs that hold per-target setup manifest + build output (for help text / rm -rf clean).
MANIFEST_CLEAN_DIRS := $(foreach p,$(NATIVE_PLATFORMS),build_$(p)) build_android \
              $(foreach k,$(DOCKER_KINDS),docker/$(k)/build)

# Repo-relative setup manifest + build.sh tree (passed to build.sh as REPO_BUILD_ROOT).
# IN_DOCKER uses docker/<kind>/build only for windows and android (vcpkg/Qt under repo). Linux in the
# devcontainer matches native: always build_linux/ (distro Qt + same layout as host make linux).
# Use `=`; expand in recipes/submakes where TARGET_PLATFORM matches the build.
REPO_BUILD_ROOT = $(if $(and $(IN_DOCKER),$(filter $(TARGET_PLATFORM),$(DOCKER_KINDS))),docker/$(TARGET_PLATFORM)/build,build_$(TARGET_PLATFORM))

.PHONY: help linux windows macos android setup setup-toolchains clean
.PHONY: installer installers linux-installer windows-installer macos-installer android-installer
# help when explicitly requested
help:
	@echo "PokerTH build targets:"
	@echo "  make                 - Default build (make linux on Linux, make macos on macOS)"
	@echo "  make setup           - Default setup (make setup-linux on Linux, make setup-macos on macOS)"
	@echo "  make linux           - Build for Linux (native)"
	@echo "  make windows         - Build for Windows (Linux: host cross-compile; macOS: Docker)"
	@echo "  make windows-docker  - Windows in Docker (run_devcontainer.py; or VS Code/Cursor devcontainer)"
	@echo "  make macos           - Build for macOS"
	@echo "  make android           - Android (Linux: host SDK/NDK/Qt; macOS: Docker)"
	@echo "  make android-docker    - Android in Docker (run_devcontainer.py; or VS Code/Cursor devcontainer)"
	@echo "  make android-installer - Placeholder (no-op; future signed APK / store flow)"
	@echo "  make linux-installer - Build + Linux AppImage"
	@echo "  make windows-installer        - Build + Windows NSIS (Linux: host; macOS: Docker)"
	@echo "  make windows-docker-installer - Windows NSIS in Docker"
	@echo "  make android-docker-installer - Android APK in Docker (placeholder)"
	@echo "  make macos-installer - Build + macOS DMG"
	@echo "  make installer       - Same as make installers"
	@echo "  make installers      - Build installers for this host (Linux: AppImage + Windows NSIS; macOS: DMG; linux-installer may fail)"
	@echo "  make setup-linux     - Install dependencies for Linux build"
	@echo "  make setup-windows   - Install dependencies for Windows cross-build"
	@echo "  make setup-macos     - Install dependencies for macOS build"
	@echo "  make setup-android   - Android SDK/NDK/Qt + vcpkg (VCPKG_DIR required; e.g. VCPKG_DIR=build_android/vcpkg)"
	@echo "  make setup-toolchains - For all unified-image targets (android + windows): Docker build only (IN_DOCKER=1), toolchain under /opt; see docker/Dockerfile"
	@echo "  make clean           - Remove build_* (incl. build_android) and docker/windows/build/, docker/android/build/ (see MANIFEST_CLEAN_DIRS in this Makefile)"
	@echo "  Docker (*-docker): run_devcontainer.py skips docker build if the image tag already exists — editing docker/Dockerfile does not by itself trigger a rebuild. Force: DOCKER_FORCE_BUILD=1 or docker rmi <image>."
	@echo "  Dev Container: do not run 'make *-docker' inside the container; use 'make android' or 'make windows' (see docs/building-developer.md)."
	@echo ""
	@echo "Host directory convention (MANIFEST_CLEAN_DIRS, REPO_BUILD_ROOT in this Makefile):"
	@echo "  Non-Docker: build_<platform>/ — .manifest.env, CMake, deploy (e.g. build_linux/, build_android/)."
	@echo "  IN_DOCKER + windows|android: docker/<kind>/build/ — vcpkg/Qt, .manifest.env, outputs. IN_DOCKER + linux (devcontainer): build_linux/ (same as native; run make setup-linux first)."
	@echo "  More: docs/building-developer.md"
	@echo ""
	@echo "Windows: 'make windows' uses host toolchain on Linux, Docker on macOS. Use 'make windows-docker' to always use Docker."
	@echo "Android: 'make android' on Linux uses build_android/$(MANIFEST_NAME); macOS Docker uses docker/android/build/$(MANIFEST_NAME)."

# Setup manifest = BUILD_DIR/$(MANIFEST_NAME). Written by setup_*.sh when setup finishes.
# Depend on scripts and requirement files so changing them (e.g. versions.env, windows-apt-packages.txt) triggers re-setup.
# Stem ($*) is the real context: linux/windows/macos/android for native, windows/android for docker.

define run_manifest_setup
	@mkdir -p $(@D)
	@if [ -n "$${SETUP_ALREADY_DONE:-}" ]; then \
	  test -s "$@" || { echo >&2 "pokerth: SETUP_ALREADY_DONE requires a non-empty $@ (populate with setup or copy from a prior machine)."; exit 1; }; \
	else \
	  TARGET_PLATFORM=$* BUILD_DIR=$(@D) $(SCRIPTS)/setup.sh && test -s "$@" || { echo >&2 "pokerth: setup did not create non-empty $@"; exit 1; }; \
	fi
endef

# windows additional dep on setup script (windows cross-compile) and windows-apt-packages.txt
build_windows/$(MANIFEST_NAME): $(SCRIPTS)/setup_linux.sh $(SCRIPTS)/windows-apt-packages.txt
# android additional dep on setup script (Android) and android-apt-packages.txt
build_android/$(MANIFEST_NAME): $(SCRIPTS)/setup_android.sh $(SCRIPTS)/android-apt-packages.txt
# linux: setup_linux.sh (USE_AQT=no system Qt + LINUX_APT_EXTRA); Docker base runs the same script
build_linux/$(MANIFEST_NAME): $(SCRIPTS)/setup_linux.sh
# Native manifests: build_linux, build_windows, build_macos, build_android
build_%/$(MANIFEST_NAME): $(SCRIPTS)/setup.sh $(SCRIPTS)/setup_macos.sh $(SCRIPTS)/functions.sh $(SCRIPTS)/versions.env $(SCRIPTS)/apt-packages.txt
	$(run_manifest_setup)

# Docker manifests: add kind-specific prereqs on these lines only (no recipe).
docker/windows/build/$(MANIFEST_NAME): $(SCRIPTS)/setup_linux.sh $(SCRIPTS)/windows-apt-packages.txt
docker/android/build/$(MANIFEST_NAME): $(SCRIPTS)/setup_android.sh $(SCRIPTS)/android-apt-packages.txt
docker/%/build/$(MANIFEST_NAME): $(SCRIPTS)/setup.sh $(SCRIPTS)/functions.sh $(SCRIPTS)/versions.env
	$(run_manifest_setup)

# Host setup for all platforms.
setup: build_$(TARGET_PLATFORM)/$(MANIFEST_NAME)

# setup-* wrappers only create native manifests under build_<platform>/, not docker/.../build/.
# Docker: same recipe as docker/%/build/$(MANIFEST_NAME) above; Make runs it when IN_DOCKER=1 and a target
# depends on $(REPO_BUILD_ROOT)/$(MANIFEST_NAME). scripts/ensure_docker_deps.py runs setup.sh deps when vcpkg/Qt are
# not ready, then invokes make — it does not create the manifest; setup_*.sh writes it when the rule runs.
SETUP_TARGETS := setup-linux setup-windows setup-macos setup-android
.PHONY: $(SETUP_TARGETS)
$(SETUP_TARGETS): setup-%:
	$(MAKE) TARGET_PLATFORM=$* setup

# Baked into docker/Dockerfile (final stage). For each unified Docker kind: toolchain only (not host shells).
# Order matches DOCKER_KINDS (android then windows).
.PHONY: setup-toolchains
ifeq ($(IN_DOCKER),1)
setup-toolchains:
	@for k in $(DOCKER_KINDS); do \
	  TARGET_PLATFORM=$$k BUILD_DIR=docker/$$k/build IN_DOCKER=1 $(SCRIPTS)/setup.sh toolchain || exit $$?; \
	done
else
setup-toolchains:
	$(error pokerth: setup-toolchains is for docker build only (docker/Dockerfile). On the host use: make setup-android / make setup-windows)
endif

# In Docker/devcontainer, toolchains come from the image (setup-toolchains at docker build); Qt/vcpkg on first
# make android / make windows via manifest rules or ensure. On the host, run setup-android / setup-windows as needed.

# Native builds: host toolchain wrappers.
linux:
	$(MAKE) TARGET_PLATFORM=linux all

macos:
	$(MAKE) TARGET_PLATFORM=macos all

# Agnostic build helper target: host build: manifest + build.sh. Caller must set TARGET_PLATFORM=windows|android (for REPO_BUILD_ROOT).
.PHONY: __do_host_build
__do_host_build:
	$(MAKE) TARGET_PLATFORM=$(TARGET_PLATFORM) $(REPO_BUILD_ROOT)/$(MANIFEST_NAME)
	TARGET_PLATFORM=$(TARGET_PLATFORM) REPO_BUILD_ROOT=$(REPO_BUILD_ROOT) $(SCRIPTS)/build.sh

# host cross-compile on Linux; force Docker on macOS (no host toolchain).
# Use windows-docker to force Docker on any host.
windows:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) TARGET_PLATFORM=windows __do_docker
else
	$(MAKE) TARGET_PLATFORM=windows __do_host_build
endif

# Use android-docker to force Docker on any host.
android:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) TARGET_PLATFORM=android __do_docker
else
	$(MAKE) TARGET_PLATFORM=android __do_host_build
endif

#
# Targets that use docker
#

# Agnostic docker build helper target, called as submake target by %-docker and docker-%-installer.
DOCKER_GOAL ?= $(TARGET_PLATFORM)
.PHONY: __do_docker
__do_docker:
	@plat="$(TARGET_PLATFORM)"; img="$${DOCKER_IMAGE:-pokerth-$$plat-dev}"; \
	$(SCRIPTS)/run_devcontainer.py "$$img" "$(DOCKER_GOAL)"
	@case "$(DOCKER_GOAL)" in \
	   windows*) echo "Done. Check docker/windows/build/deploy/.";; \
	   android) echo "Done. Check docker/android/build/android-build/.../release/ for APK (host: build_android/android-build/...).";; \
	   *) echo "Done. DOCKER_GOAL=$(DOCKER_GOAL).";; \
	esac

#
# Use "TARGET: %-pattern:" static pattern rules over the known sets to force GNU Make to allow phony pattern rules
#

# Docker builds
DOCKER_TARGETS := windows-docker android-docker
.PHONY: $(DOCKER_TARGETS)
$(DOCKER_TARGETS): %-docker:
	$(if $(IN_DEVCONTAINER),$(error pokerth: 'make $*-docker' is for the host CLI Docker. Inside a Dev Container run 'make $*' instead. See docs/building-developer.md),)
	$(MAKE) TARGET_PLATFORM=$* __do_docker

# Docker installer builds
DOCKER_INSTALLER_TARGETS := windows-docker-installer android-docker-installer
.PHONY: $(DOCKER_INSTALLER_TARGETS)
$(DOCKER_INSTALLER_TARGETS): %-docker-installer:
	$(if $(IN_DEVCONTAINER),$(error pokerth: 'make $*-docker-installer' is for the host. Inside a Dev Container run 'make $*-installer' instead. See docs/building-developer.md),)
	$(MAKE) TARGET_PLATFORM=$* DOCKER_GOAL=$*-installer __do_docker

#
# Installer targets
#

installers installer:
ifeq ($(UNAME_S),Darwin)
    # On macOS: macos-installer only.
	$(MAKE) macos-installer
else
    # On Linux: linux-installer (may fail) + windows-installer
	-$(MAKE) linux-installer
	$(MAKE) windows-installer
endif

# Installer helper target: manifest + build.sh. Caller must set TARGET_PLATFORM=windows|android (for REPO_BUILD_ROOT).
.PHONY: __do_host_installer
__do_host_installer:
	$(MAKE) TARGET_PLATFORM=$(TARGET_PLATFORM) $(REPO_BUILD_ROOT)/$(MANIFEST_NAME)
	CREATE_INSTALLER=yes TARGET_PLATFORM=$(TARGET_PLATFORM) REPO_BUILD_ROOT=$(REPO_BUILD_ROOT) $(SCRIPTS)/build.sh

#
# Installer builds: android-installer, linux-installer, windows-installer, macos-installer.
#
android-installer:
	# intentional no-op for symmetry with windows-installer et al
	@:

linux-installer: build_linux/$(MANIFEST_NAME)
	CREATE_INSTALLER=yes TARGET_PLATFORM=linux REPO_BUILD_ROOT=build_linux $(SCRIPTS)/build.sh

windows-installer:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) windows-docker-installer
else
	$(MAKE) TARGET_PLATFORM=windows __do_host_installer
endif

macos-installer: build_macos/$(MANIFEST_NAME)
	CREATE_INSTALLER=yes $(SCRIPTS)/build_macos.sh

clean:
	@rm -rf $(foreach p,$(NATIVE_PLATFORMS),build_$(p))
	@rm -rf build_android $(foreach k,$(DOCKER_KINDS),docker/$(k)/build)
