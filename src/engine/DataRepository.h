#pragma once

#include <QList>
#include <QObject>
#include <QString>

#include "types.h"

// ---------------------------------------------------------------------------
// DataRepository — SQLite persistence layer (Model in MVC).
//
// Each public method opens a short-lived connection so the object requires
// no persistent state and is safe to call from any thread (SQLite WAL mode
// allows concurrent reads while a write is in flight on another thread).
// ---------------------------------------------------------------------------
class DataRepository : public QObject
{
    Q_OBJECT

public:
    /// Opens (or creates) the database at dbPath and initialises the schema.
    /// Passing ":memory:" is supported and used by the test suite.
    explicit DataRepository(const QString& dbPath = QStringLiteral("dupecheck.db"),
                            QObject* parent = nullptr);

    // --- Core CRUD --------------------------------------------------------

    /// Bulk upsert. Replaces any existing record for a path (INSERT OR REPLACE).
    void upsertRecords(const QList<FileRecord>& records);

    /// Returns every file whose hash is shared by two or more paths.
    QList<FileRecord> getDuplicates() const;

    // --- History / Undo ---------------------------------------------------

    /// Saves the named paths to the history ledger then removes them from
    /// file_records. Called before sending files to the Trash.
    void recordAndDelete(const QStringList& paths);

    /// Restores the most recent 'trash' history entry back to file_records
    /// and removes it from history. Returns the restored path, or "" if
    /// history is empty.
    QString undoLastTrash();

    // --- Maintenance ------------------------------------------------------

    /// Wipes file_records and defragments the database file via VACUUM.
    void clearAll();

private:
    QString m_dbPath;

    void initSchema();

    /// Returns the connection name scoped to the calling thread so each
    /// thread owns its own connection (Qt SQL requirement).
    QString connectionName() const;
};
