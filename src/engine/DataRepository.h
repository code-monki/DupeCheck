#pragma once

#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QString>

#include "types.h"

// ---------------------------------------------------------------------------
// DataRepository — SQLite persistence layer (Model in MVC).
//
// Owns a single, long-lived database connection opened on the thread that
// constructs this object (normally the main/GUI thread).  All public methods
// MUST be called on that same thread — use Qt::QueuedConnection from any
// other thread (the scan worker calls upsertRecords via a queued signal).
//
// WAL journal mode is enabled at open time, giving:
//   • One writer at a time (this object).
//   • Concurrent readers without blocking the writer.
//   • Safe for a 26TB+ file index — the DB lives on disk, never in memory.
// ---------------------------------------------------------------------------
class DataRepository : public QObject
{
    Q_OBJECT

public:
    /// Opens (or creates) the database at dbPath, enables WAL mode, and
    /// initialises the schema.  The connection remains open until destruction.
    explicit DataRepository(const QString& dbPath = QStringLiteral("dupecheck.db"),
                            QObject* parent = nullptr);

    ~DataRepository() override;

    // --- Core CRUD --------------------------------------------------------

    /// Bulk upsert — call on the owning thread, or via Qt::QueuedConnection.
    /// Replaces any existing record for a path (INSERT OR REPLACE).
    void upsertRecords(const QList<FileRecord>& records);

    /// Returns every file whose hash is shared by two or more paths.
    QList<FileRecord> getDuplicates() const;

    // --- History / Undo ---------------------------------------------------

    /// Saves the named paths to the history ledger then removes them from
    /// file_records.  Called before sending files to the Trash.
    void recordAndDelete(const QStringList& paths);

    /// Restores the most recent 'trash' history entry back to file_records
    /// and removes it from history.  Returns the restored path, or "" if
    /// history is empty.
    QString undoLastTrash();

    // --- Maintenance ------------------------------------------------------

    /// Wipes file_records and defragments the database file via VACUUM.
    void clearAll();

private:
    QString m_dbPath;
    QString m_connectionName;   ///< Unique name for the QSqlDatabase connection.

    /// Returns the open connection.  Asserts that it is valid.
    QSqlDatabase db() const;

    void initSchema();
};
