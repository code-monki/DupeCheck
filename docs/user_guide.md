# DupeCheck v8.1 User Guide

## Getting Started
1. **Start New Scan**: Click the button in the top toolbar and select a directory. 
2. **Discovery Phase**: The app will quickly walk the tree to find all files.
3. **Hashing Phase**: The progress bar indicates the SHA-256 hashing process running on a background thread. Large volumes (10TB+) may take significant time depending on I/O speed.

## Managing Duplicates
Duplicates are grouped by their content hash.
- **Smart Tagging**: The file with the most recent modification date is marked **(Newest)**.
- **Selection**: Clicking a file displays its full absolute path in the status bar at the bottom.
- **Copy Path**: Use the button next to the status bar to copy the full path for use in Terminal or Finder.

## Cleanup Actions
- **Move to Trash**: Sends the selected file to the macOS Trash (or deletes on Windows).
- **Archive**: Moves the file to a selected destination folder while preserving its record in the database.
- **Undo**: Restores the *database record* for the last trashed item. 
  *Note: You must manually move the file back from the Trash bin to the original location.*

## Filtering
Use the **Filter Results** box to search for specific file extensions (e.g., `.dmg`) or keywords in the filename. The list updates in real-time.