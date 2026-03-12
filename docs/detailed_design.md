# Detailed Design

---

## Engine

### File Discovery (`DupeEngine.discover_files`)
- Uses `Path.rglob("*")` for recursive traversal.
- Filters applied in order before yielding a path:
  1. Must be a regular file (`path.is_file()`)
  2. Must not be a symlink (`not path.is_symlink()`)
  3. Must not be zero-byte (`path.stat().st_size > 0`)
  4. Must not reside in a hidden directory or have a hidden name (any path component starting with `.`)

### Full Hashing (`DupeEngine._get_full_hash`)
- Algorithm: SHA-256 (`hashlib.sha256`)
- Read strategy: 1MB chunks (`block_size = 1048576`)
- Returns: lowercase hex digest string
- Errors: returns `""` on `PermissionError` or `OSError`

### Smart Hashing (`DupeEngine._get_smart_hash`)
- Trigger: file extension in `self.smart_extensions` (default: `.mkv .mp4 .iso .zip .tar .mov`)
- Threshold: if file size ≤ 2MB, delegates to `_get_full_hash`
- Algorithm: SHA-256 of (first 1MB + last 1MB)
- Returns: `"smart-{hexdigest}"` — the prefix is critical; it prevents collision with full-hash results

---

## Repository

### Connection Factory (`DataRepository._connect`)
- Opens SQLite connection and immediately executes `PRAGMA journal_mode=WAL`
- All public methods use `_connect()` exclusively — no bare `sqlite3.connect()` calls

### Table: `file_records`
| Column      | Type     | Constraint    |
|-------------|----------|---------------|
| hash        | TEXT     |               |
| size        | INTEGER  |               |
| path        | TEXT     | PRIMARY KEY   |
| last_seen   | DATETIME |               |

- Index: B-Tree on `hash` to accelerate the `GROUP BY hash HAVING COUNT(*) > 1` duplicate query.
- Upsert strategy: `INSERT OR REPLACE` — re-scanning a path updates its record in place.

### Table: `history`
| Column      | Type     | Constraint              |
|-------------|----------|-------------------------|
| id          | INTEGER  | PRIMARY KEY AUTOINCREMENT |
| action      | TEXT     | e.g. `'trash'`          |
| hash        | TEXT     |                         |
| size        | INTEGER  |                         |
| path        | TEXT     |                         |
| last_seen   | DATETIME |                         |
| actioned_at | DATETIME | DEFAULT `datetime('now')` |

- Populated by `record_and_delete()` before the corresponding `file_records` row is removed.
- Consumed by `undo_last_trash()`, which re-inserts the row and deletes the history entry atomically.

### VACUUM Behavior (`DataRepository.clear_all`)
- `DELETE FROM file_records` is committed inside a WAL transaction.
- A **separate autocommit connection** (`isolation_level = None`) then issues `VACUUM`.
- Rationale: SQLite prohibits `VACUUM` inside an open transaction; the two-connection pattern is the correct workaround.

---

## Controller

### Scan Lifecycle (`DupeApp`)
- `initiate_scan()` guards against concurrent scans via `self.is_scanning` flag.
- `_run_scan()` executes on a `daemon=True` background thread.
- Batch: all `(hash, size, path)` tuples are collected in memory, then written to SQLite in a single `executemany` call for efficiency.
- UI callbacks use `root.after(0, ...)` to ensure all tree updates occur on the main thread.

### Trash + History
- `trash_items(paths)`: only records paths that `send2trash` successfully handled. Failed paths are reported in the status bar and excluded from the history ledger.
- `undo_last_trash()`: calls `repo.undo_last_trash()`, refreshes the tree via `get_duplicates()`, updates status bar.

---

## View

### MainWindow Layout
- **Toolbar (top):** Start Scan | Move to Trash | Undo | [right-aligned] Filter: [entry] | Settings
- **Tree:** Two-level `ttk.Treeview`. Parent rows = hash cluster groups. Child rows = individual file paths.
- **Footer (bottom):** Status label (left) | Copy Path button (right)

### Filter (`MainWindow._on_filter_changed`)
- `StringVar` trace fires on every keystroke.
- Filters `_all_clusters` (cached on last `update_tree` call) by substring match against the full path string.
- Empty filter restores the full list. No DB query issued.

### Copy Path (`MainWindow._copy_path`)
- Only active on leaf (file) nodes; group header rows are skipped.
- Uses `root.clipboard_clear()` + `root.clipboard_append()` — Tkinter's cross-platform clipboard API.

---

## Package Structure

```
dupecheck/
├── main.py                  # Dev convenience entry point (not installed)
├── pyproject.toml
├── src/
│   ├── __init__.py
│   ├── app.py               # DupeApp controller + main() entry point
│   ├── engine/
│   │   ├── __init__.py
│   │   ├── dupe_engine.py   # DupeEngine (hashing + discovery)
│   │   └── repository.py    # DataRepository (SQLite)
│   └── ui/
│       ├── __init__.py
│       ├── main_window.py   # MainWindow
│       ├── config_dialog.py # ConfigDialog
│       └── help_window.py   # HelpWindow (reference panel)
```

- `pyproject.toml` uses `where = ["."], include = ["src*"]` so that `src` is installed as a top-level package, matching the `from src.engine...` import convention used throughout the codebase.
