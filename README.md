# DupeCheck

High-volume deduplication management console for multi-terabyte data archives.

Built with **C++17** and **Qt 6** — native UI on macOS, Linux, and Windows with a responsive MVC architecture that keeps the GUI thread free during large-scale scans.

---

## Features

| Feature | Detail |
|---|---|
| **Hybrid hashing** | Full SHA-256 for documents and code; smart head-tail hashing (first + last 1 MB) for large media containers (`.mkv`, `.iso`, `.zip`, …) to minimise I/O churn on multi-TB arrays |
| **Symlink protection** | Symbolic links are unconditionally skipped — no infinite loops, no aliased duplicates |
| **Hidden-file filtering** | Any path component beginning with `.` is silently excluded |
| **Multi-drive orchestration** | Define and manage any number of scan roots (local SSDs, external arrays, NAS mounts) in a single session |
| **SQLite persistence** | WAL-mode database with batch upserts — near-instant duplicate identification after the initial index pass |
| **Safe deletion** | Files are moved to the system Trash (macOS: Finder AppleScript; others: `QFile::moveToTrash`) — no immediate `rm` |
| **Undo** | Last trash action is reverted from the history ledger; re-stage the file from Trash manually if needed |
| **Real-time filter** | Client-side substring filter on the results tree — zero DB round-trips on keystroke |

---

## Platform support

| Platform | Architecture | Packaged as |
|---|---|---|
| macOS | arm64 (Apple Silicon) | `.dmg` |
| macOS | x86_64 (Intel) | `.dmg` |
| Linux | x86_64 | `.tar.gz` |
| Linux | aarch64 | `.tar.gz` |
| Windows | x86_64 (MSVC 2022) | `.zip` |

Pre-built binaries for tagged releases are attached to the [GitHub Releases](https://github.com/code-monki/DupeCheck/releases) page.

---

## Project structure

```
.github/workflows/ci.yml   CI/CD — matrix build + release packaging
CMakeLists.txt             Root CMake build
CMakePresets.json          Per-platform configure presets
Makefile                   Developer convenience wrapper (make help)
LICENSE                    GNU LGPLv3
docs/                      Requirements, architecture, test plan, build guide
src/
  types.h                  FileRecord struct shared across all layers
  main.cpp                 QApplication entry point
  engine/
    DupeEngine             File discovery, hashing (full + smart)
    DataRepository         SQLite persistence (WAL, upsert, undo, VACUUM)
  app/
    DupeApp                Controller — QtConcurrent scan worker, trash, undo
  ui/
    MainWindow             Two-level cluster tree, toolbar, filter, copy path
    ConfigDialog           Scan paths, smart-check extensions, DB maintenance
    HelpWindow             Read-only reference panel
tests/
  test_engine.cpp          QtTest suite — hashing, discovery, filters
  test_repository.cpp      QtTest suite — SQLite CRUD, WAL, undo, VACUUM
```

---

## Quick start (macOS)

```bash
# Prerequisites: Qt 6.9+ installed via the Qt Online Installer, Ninja via Homebrew
brew install ninja

# Configure + build (debug, tests enabled)
make

# Run the unit tests
make test

# Launch the app
make run
```

See [docs/building.md](docs/building.md) for full dependency lists and platform-specific instructions.

---

## Runtime notes

- **macOS Full Disk Access** — grant Full Disk Access to Terminal (or your IDE) in  
  *System Settings → Privacy & Security → Full Disk Access* to allow scanning `/Volumes`.
- **Database location** — `dupecheck.db` is created in the working directory on first launch.  
  It is excluded from version control by `.gitignore`.
- **Smart Check extensions** — configure in *Settings*; the default set is  
  `.mkv .mp4 .iso .zip .tar .mov`.

---

## License

GNU Lesser General Public License v3.0 — see [LICENSE](LICENSE).