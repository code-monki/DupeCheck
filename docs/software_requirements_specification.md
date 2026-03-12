# System Requirements Specification (SRS)
**Version: 1.2**

**Status:** Approved

---

### Functional Requirements:

**FR-1:** The system shall identify duplicate files using SHA-256 hashing.

**FR-2:** The system shall implement "Smart Check" (Head-Tail) hashing for files with user-defined extensions to optimize I/O. For files larger than 2MB, only the first and last 1MB are hashed; the resulting digest is prefixed with `smart-` to distinguish it from a full-file SHA-256.

**FR-3:** The system shall ignore symbolic links to prevent data recursion or accidental deletion.

**FR-4:** The system shall support scanning multiple independent mount points in a single session.

**FR-5:** The system shall provide a non-destructive "Move to Trash" capability using the platform's native Trash mechanism.

**FR-6:** The system shall maintain a persistent `history` ledger. Before any file record is removed from `file_records` (e.g., on Trash), the record shall be saved to the `history` table with a timestamped `action` tag.

**FR-7:** The system shall provide an Undo operation that restores the most recent `history` entry back to `file_records` and removes it from `history`.

**FR-8:** The system shall provide real-time filtering of the duplicate tree by filename or path substring without requiring a re-scan or database query.

**FR-9:** The system shall provide a Copy Path action that writes the selected file's full absolute path to the system clipboard.

**FR-10:** The system shall skip zero-byte files during discovery; they cannot be duplicates and pose no storage risk.

---

### Non-Functional Requirements:

**NFR-1 (Performance):** All hashing and I/O operations must be performed on a background thread to prevent UI blocking. UI updates must be dispatched via `root.after()` to ensure thread safety.

**NFR-2 (Architecture):** The system must follow a strict Model-View-Controller (MVC) pattern. Controller: `DupeApp`. View: `MainWindow`, `ConfigDialog`, `HelpWindow`. Model: `DupeEngine`, `DataRepository`.

**NFR-3 (Data Integrity):** SQLite must be opened in WAL (Write-Ahead Logging) mode on every connection to permit concurrent reads during background writes and to protect against data corruption on power loss.

**NFR-4 (Packaging):** The application must be installable as a standard Python package via `pip install -e .` and launchable via the `dupecheck` console script entry point.

**NFR-5 (Tone):** The UI and all status feedback must remain neutral and objective — no conversational language, warnings framed as catastrophe, or unnecessary confirmation dialogs.
