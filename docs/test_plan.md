# Test Plan
**Version: 1.1**

**Objective:** Validate that DupeCheck v8.1 performs accurately on multi-terabyte volumes without data loss. All tests must pass before any release build.

**Environment:** macOS (M4 Pro) with Full Disk Access enabled. Run via `.venv/bin/pytest tests/`.

---

## Unit Tests — Engine (`DupeEngine`)

| Test ID | Description | Pass Criteria |
|---|---|---|
| `test_symlink_protection` | Create a real file and a symlink pointing to it; run `discover_files`. | Only the real file is yielded; symlink is absent from results. |
| `test_zero_byte_filter` | Create a zero-byte file alongside a non-empty file; run `discover_files`. | Only the non-empty file is yielded. |
| `test_hidden_file_filter` | Create a file named `.DS_Store` and a visible file; run `discover_files`. | Only the visible file is yielded. |
| `test_full_hash_logic` | Hash a known file with `_get_full_hash`. | Returns a 64-char lowercase hex string matching `sha256sum` output. |
| `test_smart_hash_logic` | Hash a file > 2MB with a `.mkv` extension using `_get_smart_hash`. | Returns a string prefixed with `smart-` followed by a 64-char hex digest. |
| `test_smart_hash_small_file_fallback` | Hash a `.mkv` file ≤ 2MB via `get_hash`. | Falls back to full hash; result does NOT have `smart-` prefix. |
| `test_smart_hash_extension_case` | Call `get_hash` on a file named `VIDEO.MKV` (uppercase extension). | Smart hash triggers (extension matching is case-insensitive). |

---

## Unit Tests — Repository (`DataRepository`)

| Test ID | Description | Pass Criteria |
|---|---|---|
| `test_wal_mode` | Open a fresh DB; query `PRAGMA journal_mode`. | Returns `wal`. |
| `test_upsert_and_duplicates` | Insert two records with the same hash; call `get_duplicates`. | Both records are returned. |
| `test_upsert_idempotent` | Insert the same path twice with different hashes; call `get_duplicates`. | Only the latest record exists (INSERT OR REPLACE behavior). |
| `test_record_and_delete` | Upsert a record, call `record_and_delete`, then query `file_records`. | Record absent from `file_records`; row present in `history` with `action='trash'`. |
| `test_undo_last_trash` | After `record_and_delete`, call `undo_last_trash`. | Record restored to `file_records`; `history` table is empty; return value matches original path. |
| `test_undo_empty_history` | Call `undo_last_trash` on empty history. | Returns `None` without error. |
| `test_clear_all_and_vacuum` | Upsert records, call `clear_all`. | `file_records` is empty; no exception raised; DB file is valid SQLite (VACUUM completed). |

---

## Integration Tests

| Test ID | Description | Pass Criteria |
|---|---|---|
| `test_multi_drive_scan` | Configure two `scan_paths` with duplicate files across both; run `_run_scan`. | `get_duplicates` returns cross-path clusters containing both paths. |
| `test_trash_removes_from_db` | Trash a file via `trash_items`; query `get_duplicates`. | The trashed path is absent from `file_records`; present in `history`. |
| `test_undo_refreshes_tree` | After trash + undo, call `get_duplicates`. | Cluster reappears if sibling still exists in `file_records`. |
| `test_filter_no_db_query` | Populate tree; apply filter string; inspect `get_duplicates` call count. | No additional DB queries issued during filtering (client-side only). |

---

## Known Gaps (Backlog)

- `test_ui_responsiveness`: Verify no UI freeze during a scan of 100k+ files. Requires instrumented scan with mock I/O.
- `test_trash_execution`: End-to-end test requires macOS Trash integration and teardown logic to restore files.
- `test_copy_path_clipboard`: Tkinter clipboard is not accessible in headless test environments; requires manual verification or a GUI test harness.
