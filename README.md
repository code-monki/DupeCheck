# DupeCheck v8.1
High-Volume Deduplication Management Console

## Overview
DupeCheck is a high-performance utility designed for managing multi-terabyte data archives (26TB+). It balances data integrity with I/O efficiency by utilizing a Hybrid Hashing approach and a persistent SQLite repository.

## Key Features

- **Hybrid Hashing:** * Full Hash: SHA-256 for documents, code, and critical files.
- **Smart Check:** Head-Tail hashing (1MB buffers) for massive media and container files (.mkv, .iso, .zip) to minimize I/O churn.
- **Symlink Awareness:** Strictly ignores symbolic links to prevent infinite loops and protect master-file integrity during large-scale migrations.
- **Multi-Drive Orchestration:** Define and manage multiple scan targets (local SSDs, external mechanical arrays) within a single session.
- **SQLite Persistence:** Uses a local database with VACUUM maintenance for near-instant duplicate identification across massive file sets.
- **Safety Net:** Utilizes send2trash for native OS hooks, ensuring "deletion" is a reversible move to the Recycle Bin/Trash.

---

## Project Structure

- src/app.py: **The Controller**. Coordinates background threading and links the Engine to the UI.
- src/engine/dupe_engine.py: **The Model (Logic)**. Handles file discovery, symlink protection, and the Smart Check hashing.
- src/engine/repository.py: **The Model (Data)**. Manages the SQLite schema, batch upserts, and grouping queries.
- src/ui/main_window.py: **The View**. High-density Treeview for cluster management and context menus.
- src/ui/config_dialog.py: **The View (Config)**. Modal for drive selection and database maintenance.

---

## Setup & Installation

1. Environment Configuration
Ensure you are using Python 3.12+ for the best compatibility with current standard library hashing:

```python
python3 -m venv venv
source venv/bin/activate
```

---

## Setup & Installation

#### 1. **Environment Configuration**

Ensure you are using Python 3.12+ for the best compatibility with current standard library hashing:

```python
python3 -m venv venv
source venv/bin/activate
```

#### 2. **Install Dependencies**

```python
pip install -r requirements.txt
```

---

## Usage

### Running the Application

From the root directory:

```python
python3 -m src.app
```

## Maintenance

- **Drive Setup:** Click the ⚙ icon (Settings) to add your external volumes or internal mount points.
- **Smart Check:** Add extensions (e.g., iso, mkv) in Settings to enable high-speed partial hashing for those types.
- **Cache Purge:** Use "Clear Scan Cache" in Settings to reset the index and perform a fresh 26TB scan.

---

## Technical Considerations

- **Buffered I/O:** The engine uses 1MB block reads to align with modern SSD and mechanical drive controller buffers, maximizing throughput.
- **MVC Architecture:** Separation of concerns ensures that the UI remains responsive even during high-latency I/O operations on external arrays.
- **Full Disk Access:** On macOS, grant Full Disk Access to your Terminal or IDE in System Settings > Privacy & Security to allow crawling of /Volumes.

---

### Deployment Checklist

1. **Package Integrity:** Ensure src/engine/__init__.py exists to allow package-level imports.
2. **Database Path:** The default index is stored as dupecheck.db in the project root.
3. **Cleanup:** Ensure the old logic folder and main.py entry point have been removed to avoid namespace collisions.

---