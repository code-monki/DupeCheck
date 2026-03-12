// test_repository.cpp — Unit tests for DataRepository (mirrors Python test_plan.md)
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
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

// Returns a unique temp-file path inside dir (the QTemporaryDir owns the dir).
static QString tmpDb(const QTemporaryDir& dir, const QString& name = QStringLiteral("test.db"))
{
    return dir.filePath(name);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void TestDataRepository::test_wal_mode()
{
    // Use a real disk file — SQLite silently ignores WAL on :memory: databases.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DataRepository repo(tmpDb(dir));

    // Open a separate raw connection to the same file and verify the pragma.
    const QString connName = QStringLiteral("wal_check");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(tmpDb(dir));
        QVERIFY(db.open());
        QSqlQuery q(db);
        q.exec(QStringLiteral("PRAGMA journal_mode"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("wal"));
        db.close();
    }
    QSqlDatabase::removeDatabase(connName);
}

void TestDataRepository::test_upsert_and_duplicates()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DataRepository repo(tmpDb(dir));

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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DataRepository repo(tmpDb(dir));

    const QString path = QStringLiteral("/x/file.txt");
    repo.upsertRecords({ makeRecord(QStringLiteral("hash_v1"), 100, path) });
    repo.upsertRecords({ makeRecord(QStringLiteral("hash_v2"), 200, path) });

    // No duplicate cluster — only one record (the replaced one).
    QCOMPARE(repo.getDuplicates().size(), 0);
}

void TestDataRepository::test_record_and_delete()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DataRepository repo(tmpDb(dir));

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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DataRepository repo(tmpDb(dir));

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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DataRepository repo(tmpDb(dir));
    QVERIFY(repo.undoLastTrash().isEmpty());
}

void TestDataRepository::test_clear_all_and_vacuum()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DataRepository repo(tmpDb(dir));

    repo.upsertRecords({
        makeRecord(QStringLiteral("h1"), 100, QStringLiteral("/v/a.txt")),
        makeRecord(QStringLiteral("h1"), 100, QStringLiteral("/v/b.txt")),
    });

    repo.clearAll();

    QCOMPARE(repo.getDuplicates().size(), 0);
}

QTEST_GUILESS_MAIN(TestDataRepository)
#include "test_repository.moc"

