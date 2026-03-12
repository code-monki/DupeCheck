# Requirements Traceability Matrix

| Req ID | Requirement Description        | Design Component                          | Test Case ID                    |
|--------|-------------------------------|-------------------------------------------|---------------------------------|
| FR-1   | SHA-256 Full Hashing           | `DupeEngine._get_full_hash`               | `test_full_hash_logic`          |
| FR-2   | Smart Check (Head-Tail)        | `DupeEngine._get_smart_hash`              | `test_smart_hash_logic`         |
| FR-3   | Ignore Symlinks                | `DupeEngine.discover_files`               | `test_symlink_protection`       |
| FR-4   | Multi-Path Support             | `DupeApp.scan_paths`                      | `test_multi_drive_scan`         |
| FR-5   | Native Trash Hook              | `DupeApp.trash_items`                     | `test_trash_execution`          |
| FR-6   | History Ledger (pre-trash)     | `DataRepository.record_and_delete`        | `test_record_and_delete`        |
| FR-7   | Undo Last Trash                | `DataRepository.undo_last_trash`, `DupeApp.undo_last_trash` | `test_undo_last_trash` |
| FR-8   | Real-Time Filter               | `MainWindow._on_filter_changed`           | `test_filter_no_db_query`       |
| FR-9   | Copy Path to Clipboard         | `MainWindow._copy_path`                   | `test_copy_path_clipboard`      |
| FR-10  | Zero-Byte File Filter          | `DupeEngine.discover_files`               | `test_zero_byte_filter`         |
| NFR-1  | Background Threading           | `DupeApp.initiate_scan`                   | `test_ui_responsiveness`        |
| NFR-2  | MVC Architecture               | `DupeApp`, `MainWindow`, `DupeEngine`, `DataRepository` | Architecture review  |
| NFR-3  | WAL Journaling Mode            | `DataRepository._connect`                 | `test_wal_mode`                 |
| NFR-4  | Installable Package + CLI      | `pyproject.toml` `[project.scripts]`      | `pip install -e . && dupecheck` |
| NFR-5  | Neutral UI Tone                | All `MainWindow` status messages          | Manual review                   |
