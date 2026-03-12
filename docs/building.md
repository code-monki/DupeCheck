# Building DupeCheck

This document covers every dependency, tool, and step required to build DupeCheck
from source on all supported platforms.

---

## Contents

1. [Dependencies summary](#1-dependencies-summary)
2. [macOS](#2-macos)
3. [Linux — x86_64](#3-linux--x86_64)
4. [Linux — aarch64](#4-linux--aarch64)
5. [Windows](#5-windows)
6. [CMake presets reference](#6-cmake-presets-reference)
7. [Makefile reference](#7-makefile-reference)
8. [Running the tests](#8-running-the-tests)
9. [CI/CD overview](#9-cicd-overview)

---

## 1. Dependencies summary

| Dependency | Version | Where to get it |
|---|---|---|
| Qt | 6.9+ | [Qt Online Installer](https://www.qt.io/download-qt-installer) |
| CMake | 3.25+ | <https://cmake.org/download/> or platform package manager |
| Ninja | 1.11+ | Platform package manager |
| C++ compiler | C++17 | AppleClang 14+ / GCC 11+ / MSVC 2022 |
| Git | any | <https://git-scm.com> |

Qt modules used: `Core`, `Gui`, `Widgets`, `Sql`, `Concurrent`, `Test` (tests only).  
No other third-party libraries are required — SQLite is bundled with Qt.

---

## 2. macOS

### 2.1 Required Qt modules

Install Qt 6.9+ via the **Qt Online Installer**.  
When prompted for components, select at minimum:

- `Qt 6.9.x → macOS` (includes all required modules)

The installer places Qt at `~/Qt/6.9.x/macos` by default — this is the path
baked into the `macos-*` presets.

### 2.2 Compiler

The Xcode Command Line Tools provide AppleClang.  Install or update with:

```bash
xcode-select --install
```

### 2.3 Build tools

```bash
# Homebrew — https://brew.sh
brew install cmake ninja
```

> The `macos-*` CMake presets hard-code `/opt/homebrew/bin/cmake`.
> If cmake is installed elsewhere, either update `CMakePresets.json` or use
> the Makefile which discovers cmake on `PATH` at `make info`.

### 2.4 Build

```bash
# Clone
git clone https://github.com/code-monki/DupeCheck.git
cd DupeCheck

# Configure + build (debug, tests enabled)
make                   # or: cmake --preset macos-debug && cmake --build build/debug

# Run tests
make test              # or: ctest --preset macos-debug

# Build release
make CONFIG=release    # or: cmake --preset macos-release && cmake --build build/release

# Launch
make run
```

### 2.5 Full Disk Access (runtime)

To scan `/Volumes` and other protected paths, grant **Full Disk Access** to your
terminal application:

*System Settings → Privacy & Security → Full Disk Access → + your terminal*

---

## 3. Linux — x86_64

### 3.1 System packages

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake ninja-build git \
    libgl1-mesa-dev libegl1-mesa-dev \
    libxkbcommon-dev libxkbcommon-x11-dev \
    libfontconfig1-dev libfreetype-dev \
    libx11-dev libxext-dev libxrender-dev libxi-dev \
    libxcb1-dev libx11-xcb-dev libxcb-glx0-dev \
    libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev \
    libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev \
    libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev \
    libxcb-xinerama0-dev libxcb-xkb-dev \
    libatspi2.0-dev
```

> On Fedora/RHEL replace `apt-get` with `dnf` and adjust package names accordingly.

### 3.2 Qt

Install Qt 6.9+ via the Qt Online Installer and select:

- `Qt 6.9.x → Desktop gcc 64-bit`

### 3.3 Build

```bash
export QTDIR=~/Qt/6.9.3/gcc_64   # adjust to your actual installation path

git clone https://github.com/code-monki/DupeCheck.git
cd DupeCheck

make                  # uses linux-debug preset
make test
make CONFIG=release
```

---

## 4. Linux — aarch64

### 4.1 System packages

Same list as §3.1 — install via `apt-get` on an aarch64 host (Raspberry Pi 5,
Ampere workstation, AWS Graviton VM, etc.).

### 4.2 Qt

In the Qt Online Installer, select:

- `Qt 6.9.x → Desktop gcc ARM 64-bit`

### 4.3 Build

```bash
export QTDIR=~/Qt/6.9.3/gcc_arm64

git clone https://github.com/code-monki/DupeCheck.git
cd DupeCheck

make
make test
```

---

## 5. Windows

All steps run from a **VS 2022 x64 Native Tools Command Prompt**
(`vcvarsall.bat amd64` must be sourced before CMake).

### 5.1 Prerequisites

| Tool | Download |
|---|---|
| Visual Studio 2022 | <https://visualstudio.microsoft.com/> — install the **Desktop development with C++** workload |
| CMake 3.25+ | bundled with VS2022, or <https://cmake.org/download/> |
| Ninja | bundled with VS2022, or `winget install Ninja-build.Ninja` |
| Qt 6.9+ | Qt Online Installer → `Qt 6.9.x → MSVC 2022 64-bit` |

### 5.2 Build

Open **x64 Native Tools Command Prompt for VS 2022**, then:

```cmd
set QTDIR=C:\Qt\6.9.3\msvc2022_64

git clone https://github.com/code-monki/DupeCheck.git
cd DupeCheck

cmake --preset windows-debug
cmake --build build\debug --parallel

ctest --test-dir build\debug --output-on-failure

cmake --preset windows-release
cmake --build build\release --parallel
```

### 5.3 Deployment

After building release, bundle Qt DLLs alongside the executable using
`windeployqt`:

```cmd
md staging
copy build\release\dupecheck.exe staging\
%QTDIR%\bin\windeployqt --release staging\dupecheck.exe
```

The `staging\` directory is self-contained and can be zipped for distribution.

---

## 6. CMake presets reference

Presets are defined in [`CMakePresets.json`](../CMakePresets.json).

| Preset | Platform | Build type | Tests |
|---|---|---|---|
| `macos-debug` | macOS | Debug | ON |
| `macos-release` | macOS | Release | OFF |
| `linux-debug` | Linux | Debug | ON |
| `linux-release` | Linux | Release | OFF |
| `windows-debug` | Windows | Debug | ON |
| `windows-release` | Windows | Release | OFF |

The macOS presets hard-code `~/Qt/6.9.3/macos`.  
Linux and Windows presets read `$QTDIR` / `%QTDIR%` from the environment —
you must set this before invoking CMake or `make`.

---

## 7. Makefile reference

The `Makefile` auto-detects the host OS via `uname -s` and selects the
appropriate preset.  `cmake` must be on `PATH` or at `/opt/homebrew/bin/cmake`.

```
make [target] [CONFIG=debug|release]
```

| Target | Description |
|---|---|
| `all` | Configure + build (default) |
| `configure` | CMake configure only |
| `build` | Incremental build (auto-configures if needed) |
| `test` | Run full test suite via CTest |
| `run` / `app` | Build then launch the application |
| `clean` | Delete `build/<CONFIG>/` |
| `distclean` | Delete entire `build/` tree |
| `info` | Print resolved paths and detected platform |
| `help` | Print this summary |

---

## 8. Running the tests

The test suite uses **Qt Test** (no external framework).  Tests are headless and
require no display server — `QT_QPA_PLATFORM=offscreen` is set automatically by
CTest on CI.  To run locally without a display:

```bash
# macOS / Linux
QT_QPA_PLATFORM=offscreen make test

# Windows
set QT_QPA_PLATFORM=offscreen
ctest --test-dir build\debug --output-on-failure
```

Test files:

| File | Coverage |
|---|---|
| `tests/test_engine.cpp` | Full hash, smart hash, fallback, extension case-insensitivity, symlink filter, zero-byte filter, hidden-file filter |
| `tests/test_repository.cpp` | WAL mode, upsert, idempotent upsert, record-and-delete, undo, undo on empty history, clearAll + VACUUM |

---

## 9. CI/CD overview

GitHub Actions workflow: [`.github/workflows/ci.yml`](../.github/workflows/ci.yml)

| Event | Action |
|---|---|
| Push to `master` | Build + test all 5 platform/arch targets |
| Pull request to `master` | Same |
| Tag `v*` | Build + test + package artifacts + create GitHub Release |

Release artifacts attached per tag:

| File | Platform |
|---|---|
| `DupeCheck-macOS-arm64.dmg` | macOS Apple Silicon |
| `DupeCheck-macOS-x86_64.dmg` | macOS Intel |
| `DupeCheck-Linux-x86_64.tar.gz` | Linux x86_64 |
| `DupeCheck-Linux-aarch64.tar.gz` | Linux ARM64 |
| `DupeCheck-Windows-x86_64.zip` | Windows 10/11 x86_64 |

Tags containing a `-` (e.g. `v1.1.0-beta`) are published as **pre-releases**.
