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

.PHONY: linux windows macos setup setup-linux setup-windows setup-macos clean help
.PHONY: linux-installer windows-installer macos-installer installer installers
# help when explicitly requested
help:
	@echo "PokerTH build targets:"
	@echo "  make              - Default build (make linux on Linux, make macos on macOS)"
	@echo "  make setup        - Default setup (make setup-linux on Linux, make setup-macos on macOS)"
	@echo "  make linux        - Build for Linux (native)"
	@echo "  make windows      - Build for Windows (cross from Linux)"
	@echo "  make macos        - Build for macOS"
	@echo "  make linux-installer  - Build + Linux AppImage"
	@echo "  make windows-installer - Build + Windows NSIS installer"
	@echo "  make macos-installer   - Build + macOS DMG"
	@echo "  make installer    - Same as make installers"
	@echo "  make installers   - Build installers for this host (Linux: AppImage + Windows NSIS; macOS: DMG; linux-installer may fail)"
	@echo "  make setup-linux  - Install dependencies for Linux build"
	@echo "  make setup-windows - Install dependencies for Windows cross-build"
	@echo "  make setup-macos  - Install dependencies for macOS build"
	@echo "  make clean        - Remove build_linux/, build_windows/, build_macos/"
	@echo ""
	@echo "Examples: make   make setup   CLEAN=yes make linux   make windows-installer"

# Stamp files: created when setup finishes. make linux/windows/macos run setup if stamp is missing.
$(STAMP_SETUP_LINUX):
	$(SCRIPTS)/setup.sh
	@touch $(STAMP_SETUP_LINUX)

$(STAMP_SETUP_WINDOWS):
	TARGET_PLATFORM=windows $(SCRIPTS)/setup.sh
	@touch $(STAMP_SETUP_WINDOWS)

$(STAMP_SETUP_MACOS):
	$(SCRIPTS)/setup_macos.sh
	@touch $(STAMP_SETUP_MACOS)

# Explicit setup always runs (removes stamp first). make linux/windows/macos only run setup if stamp missing.
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

windows: $(STAMP_SETUP_WINDOWS)
	TARGET_PLATFORM=windows $(SCRIPTS)/build.sh

macos: $(STAMP_SETUP_MACOS)
	$(SCRIPTS)/build_macos.sh

linux-installer: $(STAMP_SETUP_LINUX)
	CREATE_INSTALLER=yes $(SCRIPTS)/build.sh

windows-installer: $(STAMP_SETUP_WINDOWS)
	CREATE_INSTALLER=yes TARGET_PLATFORM=windows $(SCRIPTS)/build.sh

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
