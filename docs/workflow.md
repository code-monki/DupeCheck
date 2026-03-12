# DupeCheck v8.1: System Architecture & Workflow

## 1. Overview
DupeCheck uses a decoupled **Model-View-Controller (MVC)** architecture. It is designed to handle high-volume file systems by separating metadata discovery from intensive cryptographic hashing.

## 2. The Two-Phase Scan Process

### Phase 1: Discovery (Metadata Ingestion)
* **Mechanism**: Uses `Path.rglob()` for a high-speed recursive walk.
* **Action**: Captures `path`, `filename`, `size`, and `mtime` into the SQLite `files` table.
* **Efficiency**: Ignores zero-byte files and hidden system files (starts with `.`). 
* **State Management**: Uses `INSERT OR REPLACE` logic. If a file path already exists, the database preserves the old hash unless the file is explicitly re-processed.

### Phase 2: Hashing (Data Validation)
* **Mechanism**: Background `threading.Thread` dispatched by the controller.
* **I/O Handling**: Files are read in **1MB chunks** to maintain a low memory footprint regardless of file size (e.g., 50GB ISOs).
* **Concurrency**: Uses SQLite **WAL (Write-Ahead Logging)** mode to allow the UI to query the database while the background threads are writing hashes.



## 3. Data Safety & Management

### The Undo Ledger
Every destructive action (Trash) or move (Archive) is recorded in the `history` table before the main `files` record is altered. 
* **Trash**: Uses macOS AppleScript (`osascript`) to move files to the system Trash bin rather than immediate deletion.
* **Undo**: Restores the database record from the `history` ledger. *Note: Files must be manually pulled back from the macOS Trash.*

### Smart Versioning
The UI performs a "Smart Check" during results rendering. Within any duplicate cluster, the file with the most recent `mtime` is tagged as **(Newest)**.

## 4. Key Performance Specs
* **DB Journaling**: WAL mode for concurrent Read/Write.
* **Hashing**: SHA-256 (full files) and SHA-256 head-tail prefixed `smart-` (large media files).
* **UI Threading**: All long-running I/O is dispatched to background threads; UI updates are piped through `root.after()` to ensure thread safety.