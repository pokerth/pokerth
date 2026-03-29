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

# Default target: Native builds: host toolchain.
all: build_$(TARGET_PLATFORM)/.stamp_setup
	@echo "Building for $(TARGET_PLATFORM)"
	@TARGET_PLATFORM=$(TARGET_PLATFORM) REPO_BUILD_ROOT=$(REPO_BUILD_ROOT) $(SCRIPTS)/build.sh

SCRIPTS := ./scripts

# Stamp roots:
# - host: build_linux/, build_windows/, build_macos/, build_android/
# - docker: docker/windows/build/, docker/android/build/
#
# `ensure_docker_deps.py` sets `IN_DOCKER=1` when running `make` inside containers.
NATIVE_PLATFORMS   := linux windows macos
DOCKER_KINDS_STAMP := windows android

STAMP_DIRS := $(foreach p,$(NATIVE_PLATFORMS),build_$(p)) build_android \
              $(foreach k,$(DOCKER_KINDS_STAMP),docker/$(k)/build)

# Repo-relative setup stamp + build.sh tree (passed to build.sh as REPO_BUILD_ROOT).
# IN_DOCKER -> docker/$(TARGET_PLATFORM)/build, else build_$(TARGET_PLATFORM).
# Use `=`; expand in recipes/submakes where TARGET_PLATFORM matches the build.
REPO_BUILD_ROOT = $(if $(IN_DOCKER),docker/$(TARGET_PLATFORM)/build,build_$(TARGET_PLATFORM))

.PHONY: help linux windows macos android setup clean
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
	@echo "  make clean           - Remove build_* (incl. build_android) and docker/windows/build/, docker/android/build/ (see STAMP_DIRS in this Makefile)"
	@echo "  Docker (*-docker or devcontainer): run_devcontainer.py skips docker build if the image tag exists (default). Force rebuild: POKERTH_DOCKER_FORCE_BUILD=1."
	@echo ""
	@echo "Host directory convention (STAMP_DIRS, REPO_BUILD_ROOT in this Makefile):"
	@echo "  Non-Docker: build_<platform>/ — stamps, CMake, deploy (e.g. build_linux/, build_android/)."
	@echo "  Docker: docker/<kind>/build/ — vcpkg/Qt mounts, stamps, outputs (REPO_BUILD_ROOT when IN_DOCKER=1)."
	@echo "  More: docs/building-developer.md"
	@echo ""
	@echo "Windows: 'make windows' uses host toolchain on Linux, Docker on macOS. Use 'make windows-docker' to always use Docker."
	@echo "Android: 'make android' on Linux runs build_android/.stamp_setup; macOS uses Docker which uses docker/android/build/.stamp_setup."

# Stamp = BUILD_DIR/.stamp_setup. Created when setup finishes.
# Depend on scripts and requirement files so changing them (e.g. versions.env, windows-apt-packages.txt) triggers re-setup.
# Pattern stamp rules (single template per stamp tree).
# Stem ($*) is the real context: linux/windows/macos/android for native, windows/android for docker.

# windows additional dep on windows-apt-packages.txt
build_windows/.stamp_setup: $(SCRIPTS)/windows-apt-packages.txt
# android additional dep on android-apt-packages.txt
build_android/.stamp_setup: $(SCRIPTS)/android-apt-packages.txt
# Native stamps: build_linux/.stamp_setup, build_windows/.stamp_setup, build_macos/.stamp_setup, build_android/.stamp_setup
build_%/.stamp_setup: $(SCRIPTS)/setup.sh $(SCRIPTS)/setup_linux.sh $(SCRIPTS)/setup_macos.sh $(SCRIPTS)/setup_android.sh $(SCRIPTS)/functions.sh $(SCRIPTS)/versions.env
	@mkdir -p $(@D)
	@if [ -n "$${SETUP_ALREADY_DONE:-}" ]; then touch $@; else \
	  TARGET_PLATFORM=$* BUILD_DIR=$(@D) $(SCRIPTS)/setup.sh && touch $@; fi

# docker/windows/build additional dep on windows-apt-packages.txt
docker/windows/build/.stamp_setup: $(SCRIPTS)/windows-apt-packages.txt
# android additional dep on android-apt-packages.txt
docker/android/build/.stamp_setup: $(SCRIPTS)/android-apt-packages.txt
# Docker stamps: docker/windows/build/.stamp_setup and docker/android/build/.stamp_setup
docker/windows/build/.stamp_setup: $(SCRIPTS)/setup_linux.sh # We build windows targets on linux host, so we need to use the linux setup script
docker/android/build/.stamp_setup: $(SCRIPTS)/setup_android.sh
# stem rule needs the recipe, since it depends on $*
docker/%/build/.stamp_setup: $(SCRIPTS)/setup.sh $(SCRIPTS)/functions.sh $(SCRIPTS)/versions.env
	@mkdir -p $(@D)
	@if [ -n "$${SETUP_ALREADY_DONE:-}" ]; then touch $@; else \
	  TARGET_PLATFORM=$* BUILD_DIR=$(@D) $(SCRIPTS)/setup.sh && touch $@; fi

# Host setup for all platforms.
setup: build_$(TARGET_PLATFORM)/.stamp_setup

# setup-* wrappers only create native stamps (build_<platform>/.stamp_setup), not docker/.../build/.stamp_setup.
# Docker stamps: same recipe as docker/%/build/.stamp_setup above; Make runs it when IN_DOCKER=1 and a target
# depends on $(REPO_BUILD_ROOT)/.stamp_setup. scripts/ensure_docker_deps.py may run setup.sh directly and touch
# docker/<kind>/build/.stamp_setup without invoking this recipe (equivalent outcome).
SETUP_TARGETS := setup-linux setup-windows setup-macos setup-android
.PHONY: $(SETUP_TARGETS)
$(SETUP_TARGETS): setup-%:
	$(MAKE) TARGET_PLATFORM=$* setup

# Native builds: host toolchain wrappers.
linux:
	$(MAKE) TARGET_PLATFORM=linux all

macos:
	$(MAKE) TARGET_PLATFORM=macos all

# Agnostic build helper target: host build: stamp + build.sh. Caller must set TARGET_PLATFORM=windows|android (for REPO_BUILD_ROOT).
.PHONY: __do_host_build
__do_host_build:
	$(MAKE) TARGET_PLATFORM=$(TARGET_PLATFORM) $(REPO_BUILD_ROOT)/.stamp_setup
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
	$(SCRIPTS)/run_devcontainer.py "$$img" "docker/$$plat/.devcontainer/Dockerfile" "$(DOCKER_GOAL)"
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
	$(MAKE) TARGET_PLATFORM=$* __do_docker

# Docker installer builds
DOCKER_INSTALLER_TARGETS := windows-docker-installer android-docker-installer
.PHONY: $(DOCKER_INSTALLER_TARGETS)
$(DOCKER_INSTALLER_TARGETS): %-docker-installer:
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

# Installer helper target: stamp + build.sh. Caller must set TARGET_PLATFORM=windows|android (for REPO_BUILD_ROOT).
.PHONY: __do_host_installer
__do_host_installer:
	$(MAKE) TARGET_PLATFORM=$(TARGET_PLATFORM) $(REPO_BUILD_ROOT)/.stamp_setup
	CREATE_INSTALLER=yes TARGET_PLATFORM=$(TARGET_PLATFORM) REPO_BUILD_ROOT=$(REPO_BUILD_ROOT) $(SCRIPTS)/build.sh

#
# Installer builds: android-installer, linux-installer, windows-installer, macos-installer.
#
android-installer:
	# intentional no-op for symmetry with windows-installer et al
	@:

linux-installer: build_linux/.stamp_setup
	CREATE_INSTALLER=yes TARGET_PLATFORM=linux REPO_BUILD_ROOT=build_linux $(SCRIPTS)/build.sh

windows-installer:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) windows-docker-installer
else
	$(MAKE) TARGET_PLATFORM=windows __do_host_installer
endif

macos-installer: build_macos/.stamp_setup
	CREATE_INSTALLER=yes $(SCRIPTS)/build_macos.sh

clean:
	@rm -rf $(foreach p,$(NATIVE_PLATFORMS),build_$(p))
	@rm -rf build_android $(foreach k,$(DOCKER_KINDS_STAMP),docker/$(k)/build)
	@rm -f $(addsuffix /.stamp_setup,$(STAMP_DIRS))
