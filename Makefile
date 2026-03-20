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
	@TARGET_PLATFORM=$(TARGET_PLATFORM) $(SCRIPTS)/build.sh

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

.PHONY: linux windows macos android android-installer setup setup-platform setup-linux setup-windows setup-macos setup-android clean help
.PHONY: docker windows-docker android-docker docker-windows-installer docker-android-installer
.PHONY: linux-installer windows-installer macos-installer installer installers
# help when explicitly requested
help:
	@echo "PokerTH build targets:"
	@echo "  make                 - Default build (make linux on Linux, make macos on macOS)"
	@echo "  make setup           - Default setup (make setup-linux on Linux, make setup-macos on macOS)"
	@echo "  make linux           - Build for Linux (native)"
	@echo "  make windows         - Build for Windows (Linux: host cross-compile; macOS: Docker)"
	@echo "  make windows-docker  - TARGET_PLATFORM=windows docker (Windows in Docker)"
	@echo "  make macos           - Build for macOS"
	@echo "  make android           - Android (Linux: host SDK/NDK/Qt; macOS: Docker)"
	@echo "  make android-docker    - TARGET_PLATFORM=android docker (any host)"
	@echo "  make android-installer - Placeholder (no-op; future signed APK / store flow)"
	@echo "  make linux-installer - Build + Linux AppImage"
	@echo "  make windows-installer        - Build + Windows NSIS (Linux: host; macOS: Docker)"
	@echo "  make docker-windows-installer - Windows NSIS in Docker"
	@echo "  make docker-android-installer  - Android APK in Docker (placeholder)"
	@echo "  make macos-installer - Build + macOS DMG"
	@echo "  make installer       - Same as make installers"
	@echo "  make installers      - Build installers for this host (Linux: AppImage + Windows NSIS; macOS: DMG; linux-installer may fail)"
	@echo "  make setup-linux     - Install dependencies for Linux build"
	@echo "  make setup-windows   - Install dependencies for Windows cross-build"
	@echo "  make setup-macos     - Install dependencies for macOS build"
	@echo "  make setup-android   - vcpkg Android ports (requires VCPKG_DIR; refreshes build_android/.stamp_setup)"
	@echo "  make clean           - Remove build_linux/, build_windows/, build_macos/"
	@echo "  (Docker Windows: vcpkg+Qt cache in docker/windows/vcpkg/; first run runs setup, later runs reuse it.)"
	@echo ""
	@echo "Windows: 'make windows' uses host toolchain on Linux, Docker on macOS. Use 'make windows-docker' to always use Docker."
	@echo "Android: 'make android' on Linux runs build_android/.stamp_setup; macOS uses Docker which uses docker/android/build/.stamp_setup."

# Stamp = BUILD_DIR/.stamp_setup. Created when setup finishes.
# Depend on scripts and requirement files so changing them (e.g. versions.env, windows-apt-packages.txt) triggers re-setup.
# Pattern stamp rules (single template per stamp tree).
# Stem ($*) is the real context: linux/windows/macos/android for native, windows/android for docker.

# windows additional dep on windows-apt-packages.txt
build_windows/.stamp_setup: $(SCRIPTS)/windows-apt-packages.txt
# Native stamps: build_linux/.stamp_setup, build_windows/.stamp_setup, build_macos/.stamp_setup, build_android/.stamp_setup
build_%/.stamp_setup: $(SCRIPTS)/setup.sh $(SCRIPTS)/setup_macos.sh $(SCRIPTS)/setup_android.sh $(SCRIPTS)/functions.sh $(SCRIPTS)/versions.env
	@mkdir -p $(@D)
	@if [ -n "$${SETUP_ALREADY_DONE:-}" ]; then touch $@; else \
	  TARGET_PLATFORM=$* BUILD_DIR=$(@D) $(SCRIPTS)/setup.sh; \
	  touch $@; fi

# docker/windows/build additional dep on windows-apt-packages.txt
docker/windows/build/.stamp_setup: $(SCRIPTS)/windows-apt-packages.txt
# Docker stamps: docker/windows/build/.stamp_setup and docker/android/build/.stamp_setup
docker/%/build/.stamp_setup: $(SCRIPTS)/setup.sh $(SCRIPTS)/setup_android.sh $(SCRIPTS)/functions.sh $(SCRIPTS)/versions.env
	@mkdir -p $(@D)
	@if [ -n "$${SETUP_ALREADY_DONE:-}" ]; then touch $@; else \
	  TARGET_PLATFORM=$* BUILD_DIR=$(@D) $(SCRIPTS)/setup.sh; \
	  touch $@; fi

# Host setup for all platforms.
setup: build_$(TARGET_PLATFORM)/.stamp_setup

# Host-only setup wrappers (Docker stamps are handled by ensure_docker_deps.py).
setup-%:
	$(MAKE) TARGET_PLATFORM=$* setup

# Native builds: host toolchain wrappers.
linux:
	$(MAKE) TARGET_PLATFORM=linux all

macos:
	$(MAKE) TARGET_PLATFORM=macos all

# host cross-compile on Linux; force Docker on macOS (no host toolchain).
# Use windows-docker to force Docker on any host.
windows:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) TARGET_PLATFORM=windows docker
else
    # docker/windows/build or build_windows
	$(MAKE) $(if $(IN_DOCKER),docker/windows/build,build_windows)/.stamp_setup
	TARGET_PLATFORM=windows $(SCRIPTS)/build.sh
endif

# Use android-docker to force Docker on any host.
android:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) TARGET_PLATFORM=android docker
else
    # docker/android/build or build_android
	$(MAKE) $(if $(IN_DOCKER),docker/android/build,build_android)/.stamp_setup
	TARGET=$(ANDROID_BUILD_TARGET) TARGET_PLATFORM=android $(SCRIPTS)/build.sh
endif

DOCKER_GOAL ?= $(TARGET_PLATFORM)

# Agnostic docker build, called as submake target by %-docker and docker-%-installer.
docker:
	@plat="$(TARGET_PLATFORM)"; img="$${DOCKER_IMAGE:-pokerth-$$plat-dev}"; \
	$(SCRIPTS)/run_devcontainer.py "$$img" "docker/$$plat/.devcontainer/Dockerfile" "$(DOCKER_GOAL)"
	@case "$(DOCKER_GOAL)" in \
	   windows*) echo "Done. Check docker/windows/build/deploy/.";; \
	   android) echo "Done. Check build-android-<arch>/android-build/.../release/ for APK.";; \
	   *) echo "Done. DOCKER_GOAL=$(DOCKER_GOAL).";; \
	esac

# Docker builds: windows-docker, android-docker.
%-docker:
	$(MAKE) TARGET_PLATFORM=$* docker

# Docker installer builds: docker-windows-installer, docker-android-installer.
docker-%-installer:
	$(MAKE) TARGET_PLATFORM=$* DOCKER_GOAL=$*-installer docker

installers: installer

# On Linux: linux-installer (may fail) + windows-installer. On macOS: macos-installer only.
installer:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) macos-installer
else
    # allow linux installer to fail
	-$(MAKE) linux-installer
	$(MAKE) windows-installer
endif

#
# Installer builds: android-installer, linux-installer, windows-installer, macos-installer.
#
android-installer:
	# intentional no-op for symmetry with windows-installer et al
	@:

linux-installer: build_linux/.stamp_setup
	CREATE_INSTALLER=yes TARGET_PLATFORM=linux $(SCRIPTS)/build.sh

windows-installer:
ifeq ($(UNAME_S),Darwin)
	$(MAKE) docker-windows-installer
else
	$(MAKE) $(if $(IN_DOCKER),docker/windows/build,build_windows)/.stamp_setup
	CREATE_INSTALLER=yes TARGET_PLATFORM=windows $(SCRIPTS)/build.sh
endif

macos-installer: build_macos/.stamp_setup
	CREATE_INSTALLER=yes $(SCRIPTS)/build_macos.sh

clean:
	@rm -rf $(foreach p,$(NATIVE_PLATFORMS),build_$(p))
	@rm -rf build_android $(foreach k,$(DOCKER_KINDS_STAMP),docker/$(k)/build)
	@rm -f $(addsuffix /.stamp_setup,$(STAMP_DIRS))
