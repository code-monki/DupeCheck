#include "DataRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

DataRepository::DataRepository(const QString& dbPath, QObject* parent)
    : QObject(parent)
    , m_dbPath(dbPath)
    , m_connectionName(QStringLiteral("dupecheck_%1").arg(reinterpret_cast<quintptr>(this)))
{
    // Open one persistent connection on the constructing thread.
    // All methods must be called on this same thread (use Qt::QueuedConnection
    // from worker threads — DupeApp does this for upsertRecords).
    QSqlDatabase conn = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    conn.setDatabaseName(m_dbPath);
    if (!conn.open()) {
        qCritical() << "DataRepository: cannot open database:"
                    << conn.lastError().text();
        return;
    }

    // WAL mode: one writer, concurrent readers, safe for large data sets.
    QSqlQuery q(conn);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));

    // Tune for large sequential writes (26TB index).
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    q.exec(QStringLiteral("PRAGMA cache_size=-65536")); // ~64 MB page cache
    q.exec(QStringLiteral("PRAGMA temp_store=MEMORY"));

    initSchema();
}

DataRepository::~DataRepository()
{
    // Close the connection before removing it (Qt SQL requirement).
    {
        QSqlDatabase conn = db();
        if (conn.isOpen())
            conn.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

QSqlDatabase DataRepository::db() const
{
    Q_ASSERT_X(QSqlDatabase::contains(m_connectionName),
               "DataRepository::db()",
               "Connection not found — called from wrong thread?");
    return QSqlDatabase::database(m_connectionName, /*open=*/false);
}

// ---------------------------------------------------------------------------
// Schema
// ---------------------------------------------------------------------------

void DataRepository::initSchema()
{
    QSqlQuery q(db());

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS file_records (
            hash      TEXT    NOT NULL,
            size      INTEGER NOT NULL,
            path      TEXT    PRIMARY KEY,
            last_seen DATETIME
        )
    )"));

    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_hash ON file_records(hash)"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS history (
            id          INTEGER  PRIMARY KEY AUTOINCREMENT,
            action      TEXT,
            hash        TEXT,
            size        INTEGER,
            path        TEXT,
            last_seen   DATETIME,
            actioned_at DATETIME DEFAULT (datetime('now'))
        )
    )"));
}

// ---------------------------------------------------------------------------
// Core CRUD
// ---------------------------------------------------------------------------

void DataRepository::upsertRecords(const QList<FileRecord>& records)
{
    if (records.isEmpty())
        return;

    QSqlDatabase conn = db();
    conn.transaction();

    QSqlQuery q(conn);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO file_records (hash, size, path, last_seen) "
        "VALUES (:hash, :size, :path, datetime('now'))"));

    for (const FileRecord& rec : records) {
        q.bindValue(QStringLiteral(":hash"), rec.hash);
        q.bindValue(QStringLiteral(":size"), rec.size);
        q.bindValue(QStringLiteral(":path"), rec.path);
        if (!q.exec())
            qWarning() << "upsertRecords error:" << q.lastError().text();
    }

    conn.commit();
}

QList<FileRecord> DataRepository::getDuplicates() const
{
    QList<FileRecord> result;
    QSqlQuery q(db());
    q.exec(QStringLiteral(R"(
        SELECT hash, size, path
        FROM   file_records
        WHERE  hash IN (
            SELECT hash FROM file_records
            GROUP BY hash HAVING COUNT(*) > 1
        )
        ORDER BY hash
    )"));

    while (q.next()) {
        FileRecord rec;
        rec.hash = q.value(0).toString();
        rec.size = q.value(1).toLongLong();
        rec.path = q.value(2).toString();
        result.append(rec);
    }
    return result;
}

// ---------------------------------------------------------------------------
// History / Undo
// ---------------------------------------------------------------------------

void DataRepository::recordAndDelete(const QStringList& paths)
{
    if (paths.isEmpty())
        return;

    QSqlDatabase conn = db();
    conn.transaction();

    QSqlQuery ins(conn), del(conn);
    ins.prepare(QStringLiteral(
        "INSERT INTO history (action, hash, size, path, last_seen) "
        "SELECT 'trash', hash, size, path, last_seen "
        "FROM   file_records WHERE path = :path"));
    del.prepare(QStringLiteral(
        "DELETE FROM file_records WHERE path = :path"));

    for (const QString& path : paths) {
        ins.bindValue(QStringLiteral(":path"), path);
        ins.exec();
        del.bindValue(QStringLiteral(":path"), path);
        del.exec();
    }

    conn.commit();
}

QString DataRepository::undoLastTrash()
{
    QSqlDatabase conn = db();

    QSqlQuery sel(conn);
    sel.exec(QStringLiteral(
        "SELECT id, hash, size, path, last_seen FROM history "
        "WHERE action = 'trash' ORDER BY actioned_at DESC, id DESC LIMIT 1"));

    if (!sel.next())
        return {};

    const int     hId      = sel.value(0).toInt();
    const QString hash     = sel.value(1).toString();
    const qint64  size     = sel.value(2).toLongLong();
    const QString path     = sel.value(3).toString();
    const QString lastSeen = sel.value(4).toString();

    conn.transaction();

    QSqlQuery ins(conn);
    ins.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO file_records (hash, size, path, last_seen) "
        "VALUES (:hash, :size, :path, :ls)"));
    ins.bindValue(QStringLiteral(":hash"), hash);
    ins.bindValue(QStringLiteral(":size"), size);
    ins.bindValue(QStringLiteral(":path"), path);
    ins.bindValue(QStringLiteral(":ls"),   lastSeen);
    ins.exec();

    QSqlQuery del(conn);
    del.prepare(QStringLiteral("DELETE FROM history WHERE id = :id"));
    del.bindValue(QStringLiteral(":id"), hId);
    del.exec();

    conn.commit();
    return path;
}

// ---------------------------------------------------------------------------
// Maintenance
// ---------------------------------------------------------------------------

void DataRepository::clearAll()
{
    QSqlDatabase conn = db();

    // Step 1 — delete rows inside a WAL transaction.
    conn.transaction();
    QSqlQuery(conn).exec(QStringLiteral("DELETE FROM file_records"));
    conn.commit();

    // Step 2 — VACUUM must run outside any active transaction.
    // Close and reopen the WAL connection, run VACUUM in autocommit mode.
    conn.close();
    conn.open();
    QSqlQuery(conn).exec(QStringLiteral("VACUUM"));

    // Re-apply pragmas after reconnect.
    QSqlQuery q(conn);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    q.exec(QStringLiteral("PRAGMA cache_size=-65536"));
    q.exec(QStringLiteral("PRAGMA temp_store=MEMORY"));
}

