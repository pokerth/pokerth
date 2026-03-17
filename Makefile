# Top-level build entry point for PokerTH (plan section 7).
# Default: make = make linux (on Linux) or make macos (on macOS). make setup = setup for current OS.
# Use: make linux, make windows, make macos, make setup-linux, make setup-windows, make setup-macos, make clean.
# Installers: make linux-installer, make windows-installer, make macos-installer, make installers.
# Pass env: CLEAN=yes make linux, BUILD_TARGET=pokerth_client make linux.

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  .DEFAULT_GOAL := macos
  setup: setup-macos
else
  .DEFAULT_GOAL := linux
  setup: setup-linux
endif

SCRIPTS := ./scripts
STAMP_SETUP_LINUX  := .stamp-setup-linux
STAMP_SETUP_WINDOWS := .stamp-setup-windows
STAMP_SETUP_MACOS  := .stamp-setup-macos

# Docker-based builds (windows/windows-installer on macOS or via -docker; android always).
# Override with env: IMAGE_NAME, ANDROID_BUILD_ARGS (e.g. make android ANDROID_BUILD_ARGS="--arch x86_64").
WINDOWS_IMAGE ?= pokerth-windows-dev
WINDOWS_DOCKERFILE := docker/windows/.devcontainer/Dockerfile
# Context must be repo root so Dockerfile COPY scripts/ and docs/ work
WINDOWS_CONTEXT := .
WINDOWS_DOCKER_RUN_ENV := -e VCPKG_DIR=/opt/pokerth-windows/vcpkg -e QT_OUTPUT_DIR=/opt/pokerth-windows/Qt

ANDROID_IMAGE ?= pokerth-android-dev
ANDROID_DOCKERFILE := docker/android/.devcontainer/Dockerfile
ANDROID_CONTEXT := docker/android/.devcontainer
ANDROID_DOCKER_RUN_ENV := -e ANDROID_BUILD_ARGS="$(ANDROID_BUILD_ARGS)"

# Docker build+run: use scripts/build_docker.sh for consistent args (mounts, --target, setup-if-missing).
# Usage: $(SCRIPTS)/build_docker.sh IMAGE DOCKERFILE CONTEXT MAKE_TARGET [--target NAME] [--mount HOST:GUEST] [-e K=V] [--setup-if-missing]

.PHONY: linux windows macos android setup setup-linux setup-windows setup-macos clean help
.PHONY: windows-docker windows-installer-docker android-in-docker
.PHONY: linux-installer windows-installer macos-installer installer installers
# help when explicitly requested
help:
	@echo "PokerTH build targets:"
	@echo "  make              - Default build (make linux on Linux, make macos on macOS)"
	@echo "  make setup        - Default setup (make setup-linux on Linux, make setup-macos on macOS)"
	@echo "  make linux        - Build for Linux (native)"
	@echo "  make windows      - Build for Windows (Linux: host cross-compile; macOS: Docker)"
	@echo "  make windows-docker - Build for Windows in Docker (same on Linux and macOS)"
	@echo "  make macos        - Build for macOS"
	@echo "  make android      - Build for Android in Docker (only method; Linux and macOS)"
	@echo "  make linux-installer  - Build + Linux AppImage"
	@echo "  make windows-installer - Build + Windows NSIS (Linux: host; macOS: Docker)"
	@echo "  make windows-installer-docker - Build + Windows NSIS in Docker (any host)"
	@echo "  make macos-installer   - Build + macOS DMG"
	@echo "  make installer    - Same as make installers"
	@echo "  make installers   - Build installers for this host (Linux: AppImage + Windows NSIS; macOS: DMG; linux-installer may fail)"
	@echo "  make setup-linux  - Install dependencies for Linux build"
	@echo "  make setup-windows - Install dependencies for Windows cross-build"
	@echo "  make setup-macos  - Install dependencies for macOS build"
	@echo "  make clean        - Remove build_linux/, build_windows/, build_macos/"
	@echo "  (Docker Windows: vcpkg+Qt cache in docker/windows/vcpkg/; first run runs setup, later runs reuse it.)"
	@echo ""
	@echo "Windows: 'make windows' uses host toolchain on Linux, Docker on macOS. Use 'make windows-docker' to always use Docker."
	@echo "Android: 'make android' always uses Docker (no host build)."
	@echo ""
	@echo "Examples: make   make setup   make windows-docker   make android"

# Stamp files: created when setup finishes. make linux/windows/macos run setup if stamp is missing
# or if setup scripts are newer than the stamp (force re-setup when scripts change).
$(STAMP_SETUP_LINUX): $(SCRIPTS)/setup.sh $(SCRIPTS)/functions.sh
	$(SCRIPTS)/setup.sh
	@touch $(STAMP_SETUP_LINUX)

$(STAMP_SETUP_WINDOWS): $(SCRIPTS)/setup.sh $(SCRIPTS)/functions.sh
	TARGET_PLATFORM=windows $(SCRIPTS)/setup.sh
	@touch $(STAMP_SETUP_WINDOWS)

$(STAMP_SETUP_MACOS): $(SCRIPTS)/setup_macos.sh $(SCRIPTS)/functions.sh
	$(SCRIPTS)/setup_macos.sh
	@touch $(STAMP_SETUP_MACOS)

# Explicit setup always runs (removes stamp first). make linux/windows/macos run setup if stamp missing or scripts newer.
setup-linux:
	@rm -f $(STAMP_SETUP_LINUX)
	$(MAKE) $(STAMP_SETUP_LINUX)

setup-windows:
	@rm -f $(STAMP_SETUP_WINDOWS)
	$(MAKE) $(STAMP_SETUP_WINDOWS)

setup-macos:
	@rm -f $(STAMP_SETUP_MACOS)
	$(MAKE) $(STAMP_SETUP_MACOS)

linux: $(STAMP_SETUP_LINUX)
	$(SCRIPTS)/build.sh

# Windows: host cross-compile on Linux; Docker on macOS (no host toolchain). Use windows-docker to force Docker on any host.
windows:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) windows-docker
else
	$(MAKE) $(STAMP_SETUP_WINDOWS)
	TARGET_PLATFORM=windows $(SCRIPTS)/build.sh
endif

windows-docker:
	$(SCRIPTS)/build_docker.sh $(WINDOWS_IMAGE) $(WINDOWS_DOCKERFILE) $(WINDOWS_CONTEXT) windows \
	  --target base --mount docker/windows/vcpkg:/opt/pokerth-windows \
	$(WINDOWS_DOCKER_RUN_ENV) --setup-if-missing
	@echo "Done. Check docker/windows/build/deploy/ for the Windows build."

macos: $(STAMP_SETUP_MACOS)
	$(SCRIPTS)/build_macos.sh

# Android: Docker only (no host build path). Pass ANDROID_BUILD_ARGS for build_android.sh (e.g. --arch x86_64).
android:
	$(SCRIPTS)/build_docker.sh $(ANDROID_IMAGE) $(ANDROID_DOCKERFILE) $(ANDROID_CONTEXT) android-in-docker \
	  -e ANDROID_BUILD_ARGS="$(ANDROID_BUILD_ARGS)"
	@echo "Done. Check build-android-<arch>/android-build/build/outputs/apk/release/ for the APK."

# In-docker targets: build-args pattern. Script passes ANDROID_BUILD_ARGS for android-in-docker.
android-in-docker_BUILD_ARGS ?= $(ANDROID_BUILD_ARGS)
windows_BUILD_ARGS =
windows-installer_BUILD_ARGS =

# Used only inside devcontainers (make windows-docker / android etc.). Recipe uses $($(MAKECMDGOALS)_BUILD_ARGS).
android-in-docker:
	bash docker/android/build_android.sh $($(MAKECMDGOALS)_BUILD_ARGS)

linux-installer: $(STAMP_SETUP_LINUX)
	CREATE_INSTALLER=yes $(SCRIPTS)/build.sh

windows-installer:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) windows-installer-docker
else
	$(MAKE) $(STAMP_SETUP_WINDOWS)
	CREATE_INSTALLER=yes TARGET_PLATFORM=windows $(SCRIPTS)/build.sh
endif

windows-installer-docker:
	$(SCRIPTS)/build_docker.sh $(WINDOWS_IMAGE) $(WINDOWS_DOCKERFILE) $(WINDOWS_CONTEXT) windows-installer \
	  --target base --mount docker/windows/vcpkg:/opt/pokerth-windows \
	$(WINDOWS_DOCKER_RUN_ENV) --setup-if-missing
	@echo "Done. Check docker/windows/build/deploy/ for the Windows build."

macos-installer: $(STAMP_SETUP_MACOS)
	CREATE_INSTALLER=yes $(SCRIPTS)/build_macos.sh

installer: installers

# On Linux: linux-installer (may fail) + windows-installer. On macOS: macos-installer only.
installers:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) macos-installer
else
	-$(MAKE) linux-installer
	$(MAKE) windows-installer
endif

clean:
	$(SCRIPTS)/clean_build.sh
	@rm -f $(STAMP_SETUP_LINUX) $(STAMP_SETUP_WINDOWS) $(STAMP_SETUP_MACOS)
