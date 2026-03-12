# SQLite Data Integrity and Maintenance Whitepaper

To ensure the reliability of the 26TB archive index across your home lab and mobile office setups, the system utilizes specific SQLite features to mitigate data corruption during power-loss events on mechanical arrays.

## Transactional Integrity (ACID Compliance)
The DataRepository utilizes SQLite’s native Atomic Commit capabilities. Every upsert_records operation is wrapped in a transaction, ensuring that even if the connection to an external mechanical drive is severed mid-write, the database remains in a consistent state.

## The Role of VACUUM in High-Capacity Indexing
The clear_all method explicitly invokes the VACUUM command.

- **Space Reclamation:** Simply deleting records from a large database (which your 26TB index can become) does not reduce the file size on disk; it only leaves "empty pages." VACUUM rebuilds the database into a new, compact file.
- **Defragmentation:** For mechanical drives, VACUUM reduces file fragmentation by storing the B-Tree index in contiguous pages, significantly improving search performance during duplicate grouping.
- **Corruption Recovery:** Running VACUUM is a secondary check for structural integrity, as it requires reading the entire database to rebuild it.

## Power-Loss Mitigation
For the high-capacity mechanical arrays you maintain, we rely on **Journaling Mode**. This ensures that any interrupted "Trash" or "Scan" operations can be rolled back upon the next application launch, preventing the "orphan records" often found in less rigorous file-tracking systems.