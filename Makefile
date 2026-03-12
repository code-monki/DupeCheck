# =============================================================================
#  DupeCheck — top-level build orchestrator
#
#  Wraps CMake + CTest so you never have to type a cmake invocation by hand.
#  All targets accept an optional CONFIG= override (default: debug).
#
#  Usage:
#    make [target] [CONFIG=debug|release]
#
#  Examples:
#    make                   # configure + build (debug)
#    make test              # run the unit-test suite (debug)
#    make run               # launch the app bundle (debug)
#    make CONFIG=release build
#    make CONFIG=release app
# =============================================================================

# ---------------------------------------------------------------------------
# Paths and platform detection
# ---------------------------------------------------------------------------
CMAKE      := /opt/homebrew/bin/cmake
BUILD_ROOT := $(CURDIR)/build
CONFIG     ?= debug
BUILD_DIR  := $(BUILD_ROOT)/$(CONFIG)

# Detect the host OS and derive the preset prefix + run command.
# On Linux/Windows developers must set QTDIR before using presets.
UNAME := $(shell uname -s 2>/dev/null || echo Windows)
ifeq ($(UNAME),Darwin)
  PLATFORM   := macos
  EXECUTABLE := $(BUILD_DIR)/dupecheck.app
  RUN_CMD    := open "$(EXECUTABLE)"
else ifeq ($(UNAME),Linux)
  PLATFORM   := linux
  EXECUTABLE := $(BUILD_DIR)/dupecheck
  RUN_CMD    := "$(EXECUTABLE)"
else
  PLATFORM   := windows
  EXECUTABLE := $(BUILD_DIR)/dupecheck.exe
  RUN_CMD    := "$(EXECUTABLE)"
endif

PRESET := $(PLATFORM)-$(CONFIG)

# ---------------------------------------------------------------------------
# Cosmetics
# ---------------------------------------------------------------------------
BOLD  := \033[1m
RESET := \033[0m
GREEN := \033[0;32m
CYAN  := \033[0;36m
YELLOW := \033[0;33m

# ---------------------------------------------------------------------------
# Phony declarations
# ---------------------------------------------------------------------------
.PHONY: help all configure build test run app clean distclean info

# ---------------------------------------------------------------------------
# Default target
# ---------------------------------------------------------------------------
all: configure build  ## Configure then build (default)

# ---------------------------------------------------------------------------
# help — lists every documented target
# ---------------------------------------------------------------------------
help:
	@echo ""
	@echo "  $(BOLD)DupeCheck — build orchestrator$(RESET)"
	@echo ""
	@echo "  $(CYAN)USAGE$(RESET)"
	@echo "    make [target] [CONFIG=debug|release]"
	@echo ""
	@echo "  $(CYAN)TARGETS$(RESET)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "all"        "Configure + build (default target)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "configure"  "Run cmake --preset <PLATFORM>-<CONFIG> to generate Ninja files"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "build"      "Compile all targets (requires configure)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "test"       "Run the full unit-test suite via CTest"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "run"        "Build then launch the app (macOS: open .app; Linux/Win: exec)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "app"        "Alias for 'run'"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "clean"      "Delete the build tree for CONFIG (keeps other configs)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "distclean"  "Delete the entire build/ directory (all configs)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "info"       "Print resolved paths and config values"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "help"       "Show this message"
	@echo ""
	@echo "  $(CYAN)CONFIG VALUES$(RESET)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "debug"   "CMAKE_BUILD_TYPE=Debug,   BUILD_TESTS=ON  (default)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "release" "CMAKE_BUILD_TYPE=Release, BUILD_TESTS=OFF"
	@echo ""
	@echo "  $(CYAN)PLATFORM PRESETS$(RESET) (auto-detected from uname)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "macos-debug"    "Uses ~/Qt/6.9.3/macos (hardcoded)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "macos-release"  "Uses ~/Qt/6.9.3/macos (hardcoded)"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "linux-*"        "Requires:  export QTDIR=~/Qt/6.9.3/gcc_64"
	@printf "    $(BOLD)%-18s$(RESET) %s\n" "windows-*"      "Requires:  set QTDIR=C:\\Qt\\6.9.3\\msvc2022_64"
	@echo ""
	@echo "  $(CYAN)EXAMPLES$(RESET)"
	@echo "    make                        # debug configure + build"
	@echo "    make test                   # run unit tests (debug)"
	@echo "    make run                    # build + open app (debug)"
	@echo "    make CONFIG=release build   # release build"
	@echo "    make CONFIG=release app     # release build + open app"
	@echo "    make clean                  # wipe debug build dir"
	@echo "    make distclean              # wipe all build dirs"
	@echo ""

# ---------------------------------------------------------------------------
# Configure — generates Ninja build files via the preset
# ---------------------------------------------------------------------------
configure:
	@echo "$(YELLOW)▶ Configuring [$(PRESET)]$(RESET)"
	$(CMAKE) --preset $(PRESET)

# ---------------------------------------------------------------------------
# Build — incremental, respects prior configure
# ---------------------------------------------------------------------------
build: $(BUILD_DIR)/build.ninja
	@echo "$(YELLOW)▶ Building [$(PRESET)]$(RESET)"
	$(CMAKE) --build $(BUILD_DIR) --parallel

# If the build dir exists but no build.ninja, configure first automatically.
$(BUILD_DIR)/build.ninja:
	$(MAKE) configure

# ---------------------------------------------------------------------------
# Test — runs the CTest suite for the current CONFIG
# ---------------------------------------------------------------------------
test: build
	@echo "$(YELLOW)▶ Testing [$(PRESET)]$(RESET)"
	cd $(BUILD_DIR) && ctest --output-on-failure --parallel 4

# ---------------------------------------------------------------------------
# Run / app — build then open the macOS app bundle
# ---------------------------------------------------------------------------
run app: build
	@echo "$(YELLOW)▶ Launching $(EXECUTABLE)$(RESET)"
	@if [ ! -e "$(EXECUTABLE)" ]; then \
	    echo "  ERROR: executable not found at $(EXECUTABLE)"; exit 1; \
	fi
	$(RUN_CMD)

# ---------------------------------------------------------------------------
# Clean — remove only the current CONFIG's build tree
# ---------------------------------------------------------------------------
clean:
	@echo "$(YELLOW)▶ Cleaning [$(CONFIG)] build$(RESET)"
	$(CMAKE) --build $(BUILD_DIR) --target clean 2>/dev/null || true
	rm -rf "$(BUILD_DIR)"

# ---------------------------------------------------------------------------
# Distclean — wipe everything under build/
# ---------------------------------------------------------------------------
distclean:
	@echo "$(YELLOW)▶ Removing entire build/ directory$(RESET)"
	rm -rf "$(BUILD_ROOT)"

# ---------------------------------------------------------------------------
# Info — diagnostic dump
# ---------------------------------------------------------------------------
info:
	@echo ""
	@echo "  $(BOLD)Resolved configuration$(RESET)"
	@printf "    %-20s %s\n" "PLATFORM"    "$(PLATFORM)"
	@printf "    %-20s %s\n" "CONFIG"      "$(CONFIG)"
	@printf "    %-20s %s\n" "PRESET"      "$(PRESET)"
	@printf "    %-20s %s\n" "BUILD_DIR"   "$(BUILD_DIR)"
	@printf "    %-20s %s\n" "EXECUTABLE"  "$(EXECUTABLE)"
	@printf "    %-20s %s\n" "CMAKE"       "$(CMAKE)"
	@cmake_ver=$$($(CMAKE) --version | head -1); \
	    printf "    %-20s %s\n" "cmake version" "$$cmake_ver"
	@echo ""
