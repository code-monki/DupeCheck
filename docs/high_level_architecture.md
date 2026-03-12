# High-Level Architectural

**Pattern:** Model-View-Controller (MVC)

**View Layer:** Tkinter-based management console providing high-density data views via MainWindow and ConfigDialog.

**Controller Layer:** DupeApp coordinates the lifecycle of the scan, thread management, and communication between UI and Engine.

**Model Layer:**

- **Engine:** DupeEngine handles file discovery and the hashing logic (Full vs. Smart).
- **Repository:** DataRepository manages a persistent SQLite metadata store for cross-session analysis.