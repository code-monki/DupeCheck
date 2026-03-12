#include "DataRepository.h"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QtDebug>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static QSqlDatabase openConnection(const QString& name, const QString& dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    db.setDatabaseName(dbPath);
    if (!db.open())
        qCritical() << "DataRepository: cannot open database:" << db.lastError().text();
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    return db;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

DataRepository::DataRepository(const QString& dbPath, QObject* parent)
    : QObject(parent)
    , m_dbPath(dbPath)
{
    initSchema();
}

QString DataRepository::connectionName() const
{
    // One connection per thread keeps Qt SQL happy.
    return QStringLiteral("dupecheck_%1_%2")
        .arg(m_dbPath)
        .arg(reinterpret_cast<quintptr>(QThread::currentThread()));
}

void DataRepository::initSchema()
{
    const QString name = connectionName();
    {
        QSqlDatabase db = openConnection(name, m_dbPath);
        QSqlQuery q(db);

        q.exec(QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS file_records (
                hash      TEXT,
                size      INTEGER,
                path      TEXT PRIMARY KEY,
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
    QSqlDatabase::removeDatabase(name);
}

// ---------------------------------------------------------------------------
// Core CRUD
// ---------------------------------------------------------------------------

void DataRepository::upsertRecords(const QList<FileRecord>& records)
{
    if (records.isEmpty())
        return;

    const QString name = connectionName();
    {
        QSqlDatabase db = QSqlDatabase::contains(name)
            ? QSqlDatabase::database(name)
            : openConnection(name, m_dbPath);

        db.transaction();
        QSqlQuery q(db);
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
        db.commit();
    }
}

QList<FileRecord> DataRepository::getDuplicates() const
{
    const QString name = connectionName();
    QList<FileRecord> result;
    {
        QSqlDatabase db = QSqlDatabase::contains(name)
            ? QSqlDatabase::database(name)
            : openConnection(name, m_dbPath);

        QSqlQuery q(db);
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

    const QString name = connectionName();
    {
        QSqlDatabase db = QSqlDatabase::contains(name)
            ? QSqlDatabase::database(name)
            : openConnection(name, m_dbPath);

        db.transaction();
        QSqlQuery ins(db), del(db);
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
        db.commit();
    }
}

QString DataRepository::undoLastTrash()
{
    const QString name = connectionName();
    QString restoredPath;
    {
        QSqlDatabase db = QSqlDatabase::contains(name)
            ? QSqlDatabase::database(name)
            : openConnection(name, m_dbPath);

        QSqlQuery sel(db);
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

        db.transaction();

        QSqlQuery ins(db);
        ins.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO file_records (hash, size, path, last_seen) "
            "VALUES (:hash, :size, :path, :ls)"));
        ins.bindValue(QStringLiteral(":hash"), hash);
        ins.bindValue(QStringLiteral(":size"), size);
        ins.bindValue(QStringLiteral(":path"), path);
        ins.bindValue(QStringLiteral(":ls"),   lastSeen);
        ins.exec();

        QSqlQuery del(db);
        del.prepare(QStringLiteral("DELETE FROM history WHERE id = :id"));
        del.bindValue(QStringLiteral(":id"), hId);
        del.exec();

        db.commit();
        restoredPath = path;
    }
    return restoredPath;
}

// ---------------------------------------------------------------------------
// Maintenance
// ---------------------------------------------------------------------------

void DataRepository::clearAll()
{
    // Step 1: delete rows inside a WAL transaction.
    {
        const QString name = connectionName();
        QSqlDatabase db = QSqlDatabase::contains(name)
            ? QSqlDatabase::database(name)
            : openConnection(name, m_dbPath);

        db.transaction();
        QSqlQuery(db).exec(QStringLiteral("DELETE FROM file_records"));
        db.commit();
    }

    // Step 2: VACUUM must run outside any open transaction and outside any
    // active WAL connection — open a dedicated autocommit connection.
    const QString vacName = QStringLiteral("dupecheck_vacuum_%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThread()));
    {
        QSqlDatabase vac = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), vacName);
        vac.setDatabaseName(m_dbPath);
        if (vac.open()) {
            QSqlQuery(vac).exec(QStringLiteral("VACUUM"));
            vac.close();
        }
    }
    QSqlDatabase::removeDatabase(vacName);
}
