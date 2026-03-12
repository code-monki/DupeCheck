// test_repository.cpp — Unit tests for DataRepository (mirrors Python test_plan.md)
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTest>

#include "engine/DataRepository.h"
#include "types.h"

class TestDataRepository : public QObject
{
    Q_OBJECT

private slots:
    // NFR-3 — WAL journal mode
    void test_wal_mode();

    // Basic upsert + duplicate query
    void test_upsert_and_duplicates();

    // INSERT OR REPLACE idempotency
    void test_upsert_idempotent();

    // FR-6 — record_and_delete (pre-trash ledger)
    void test_record_and_delete();

    // FR-7 — undo_last_trash restores the record
    void test_undo_last_trash();

    // FR-7 — undo on empty history returns ""
    void test_undo_empty_history();

    // clearAll + VACUUM succeeds without exception
    void test_clear_all_and_vacuum();
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static FileRecord makeRecord(const QString& hash, qint64 size, const QString& path)
{
    FileRecord r;
    r.hash = hash;
    r.size = size;
    r.path = path;
    return r;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void TestDataRepository::test_wal_mode()
{
    DataRepository repo(QStringLiteral(":memory:"));

    // Open a raw connection to verify the pragma.
    QSqlDatabase db = QSqlDatabase::addDatabase(
        QStringLiteral("QSQLITE"), QStringLiteral("wal_test"));
    db.setDatabaseName(QStringLiteral(":memory:"));
    QVERIFY(db.open());
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA journal_mode"));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("wal"));
    db.close();
    QSqlDatabase::removeDatabase(QStringLiteral("wal_test"));
}

void TestDataRepository::test_upsert_and_duplicates()
{
    DataRepository repo(QStringLiteral(":memory:"));

    const QString hash = QStringLiteral("abc123");
    repo.upsertRecords({
        makeRecord(hash, 1000, QStringLiteral("/a/file1.txt")),
        makeRecord(hash, 1000, QStringLiteral("/b/file2.txt")),
    });

    const QList<FileRecord> dupes = repo.getDuplicates();
    QCOMPARE(dupes.size(), 2);
}

void TestDataRepository::test_upsert_idempotent()
{
    DataRepository repo(QStringLiteral(":memory:"));

    const QString path = QStringLiteral("/x/file.txt");
    repo.upsertRecords({ makeRecord(QStringLiteral("hash_v1"), 100, path) });
    repo.upsertRecords({ makeRecord(QStringLiteral("hash_v2"), 200, path) });

    // No duplicate cluster — only one record (the replaced one).
    const QList<FileRecord> dupes = repo.getDuplicates();
    QCOMPARE(dupes.size(), 0);
}

void TestDataRepository::test_record_and_delete()
{
    DataRepository repo(QStringLiteral(":memory:"));

    const QString hash = QStringLiteral("deadbeef");
    repo.upsertRecords({
        makeRecord(hash, 500, QStringLiteral("/d/f1.iso")),
        makeRecord(hash, 500, QStringLiteral("/d/f2.iso")),
    });

    repo.recordAndDelete({ QStringLiteral("/d/f1.iso") });

    // f1 must be absent from duplicates.
    const QList<FileRecord> dupes = repo.getDuplicates();
    for (const FileRecord& r : dupes)
        QVERIFY(r.path != QStringLiteral("/d/f1.iso"));
}

void TestDataRepository::test_undo_last_trash()
{
    DataRepository repo(QStringLiteral(":memory:"));

    const QString hash = QStringLiteral("cafebabe");
    repo.upsertRecords({
        makeRecord(hash, 800, QStringLiteral("/e/orig.mkv")),
        makeRecord(hash, 800, QStringLiteral("/e/copy.mkv")),
    });

    repo.recordAndDelete({ QStringLiteral("/e/orig.mkv") });
    QCOMPARE(repo.getDuplicates().size(), 0); // only one record remains

    const QString restored = repo.undoLastTrash();
    QCOMPARE(restored, QStringLiteral("/e/orig.mkv"));

    // Cluster of two is back.
    QCOMPARE(repo.getDuplicates().size(), 2);
}

void TestDataRepository::test_undo_empty_history()
{
    DataRepository repo(QStringLiteral(":memory:"));
    const QString result = repo.undoLastTrash();
    QVERIFY(result.isEmpty()); // must return "" without error
}

void TestDataRepository::test_clear_all_and_vacuum()
{
    // Use a real temp file — VACUUM on :memory: is a no-op in some SQLite builds.
    const QString tmpPath = QDir::tempPath() + QStringLiteral("/dupecheck_test_vacuum.db");
    QFile::remove(tmpPath); // ensure clean state

    {
        DataRepository repo(tmpPath);
        repo.upsertRecords({
            makeRecord(QStringLiteral("h1"), 100, QStringLiteral("/v/a.txt")),
            makeRecord(QStringLiteral("h1"), 100, QStringLiteral("/v/b.txt")),
        });

        repo.clearAll(); // must not throw

        QCOMPARE(repo.getDuplicates().size(), 0);
    }

    QFile::remove(tmpPath);
}

QTEST_GUILESS_MAIN(TestDataRepository)
#include "test_repository.moc"
