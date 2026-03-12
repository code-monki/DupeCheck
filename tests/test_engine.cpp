// test_engine.cpp — Unit tests for DupeEngine (mirrors Python test_plan.md)
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include "engine/DupeEngine.h"

class TestDupeEngine : public QObject
{
    Q_OBJECT

private slots:
    // FR-3 — symlink protection
    void test_symlink_protection();

    // FR-10 — zero-byte filter
    void test_zero_byte_filter();

    // Hidden file filter
    void test_hidden_file_filter();

    // FR-1 — full hash logic
    void test_full_hash_logic();

    // FR-2 — smart hash for large file
    void test_smart_hash_logic();

    // FR-2 — smart hash falls back to full hash for small file
    void test_smart_hash_small_file_fallback();

    // FR-2 — extension matching is case-insensitive
    void test_smart_hash_extension_case();
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void writeFile(const QString& path, const QByteArray& data)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

static QByteArray randomBytes(int n)
{
    QByteArray b(n, '\0');
    for (int i = 0; i < n; ++i)
        b[i] = static_cast<char>(qrand() % 256);
    return b;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void TestDupeEngine::test_symlink_protection()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString realPath = tmp.filePath(QStringLiteral("real.txt"));
    writeFile(realPath, QByteArrayLiteral("hello"));

    const QString linkPath = tmp.filePath(QStringLiteral("link.txt"));
    QFile::link(realPath, linkPath);

    DupeEngine engine;
    QStringList found;
    QObject ctx;
    QObject::connect(&engine, &DupeEngine::fileDiscovered, &ctx,
                     [&found](const FileRecord& r) { found.append(r.path); });
    engine.scanDirectory(tmp.path());

    QVERIFY(found.contains(realPath));
    QVERIFY(!found.contains(linkPath));
}

void TestDupeEngine::test_zero_byte_filter()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString empty  = tmp.filePath(QStringLiteral("empty.txt"));
    const QString normal = tmp.filePath(QStringLiteral("normal.txt"));
    writeFile(empty,  QByteArray());
    writeFile(normal, QByteArrayLiteral("data"));

    DupeEngine engine;
    QStringList found;
    QObject ctx;
    QObject::connect(&engine, &DupeEngine::fileDiscovered, &ctx,
                     [&found](const FileRecord& r) { found.append(r.path); });
    engine.scanDirectory(tmp.path());

    QVERIFY(!found.contains(empty));
    QVERIFY(found.contains(normal));
}

void TestDupeEngine::test_hidden_file_filter()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString hidden  = tmp.filePath(QStringLiteral(".DS_Store"));
    const QString visible = tmp.filePath(QStringLiteral("visible.txt"));
    writeFile(hidden,  QByteArrayLiteral("meta"));
    writeFile(visible, QByteArrayLiteral("data"));

    DupeEngine engine;
    QStringList found;
    QObject ctx;
    QObject::connect(&engine, &DupeEngine::fileDiscovered, &ctx,
                     [&found](const FileRecord& r) { found.append(r.path); });
    engine.scanDirectory(tmp.path());

    QVERIFY(!found.contains(hidden));
    QVERIFY(found.contains(visible));
}

void TestDupeEngine::test_full_hash_logic()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QByteArray data = QByteArrayLiteral("The quick brown fox jumps over the lazy dog");
    const QString path = tmp.filePath(QStringLiteral("test.txt"));
    writeFile(path, data);

    // Expected: SHA-256 of the data.
    const QString expected = QString::fromLatin1(
        QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());

    DupeEngine engine;
    const QString result = engine.getHash(path);

    QCOMPARE(result, expected);
    QCOMPARE(result.length(), 64);
    QVERIFY(result == result.toLower());
}

void TestDupeEngine::test_smart_hash_logic()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // Create a file larger than 2 MB with .mkv extension.
    const int size = 3 * 1024 * 1024; // 3 MB
    const QByteArray data = randomBytes(size);
    const QString path = tmp.filePath(QStringLiteral("video.mkv"));
    writeFile(path, data);

    DupeEngine engine;
    const QString result = engine.getHash(path);

    QVERIFY(result.startsWith(QStringLiteral("smart-")));
    QCOMPARE(result.mid(6).length(), 64); // 64-char hex after prefix
}

void TestDupeEngine::test_smart_hash_small_file_fallback()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // Small .mkv file (< 2 MB) must fall back to full hash.
    const QByteArray data = QByteArrayLiteral("tiny mkv content");
    const QString path = tmp.filePath(QStringLiteral("small.mkv"));
    writeFile(path, data);

    DupeEngine engine;
    const QString result = engine.getHash(path);

    QVERIFY(!result.startsWith(QStringLiteral("smart-")));
    QCOMPARE(result.length(), 64);
}

void TestDupeEngine::test_smart_hash_extension_case()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const int size = 3 * 1024 * 1024;
    const QByteArray data = randomBytes(size);
    const QString path = tmp.filePath(QStringLiteral("VIDEO.MKV")); // uppercase
    writeFile(path, data);

    DupeEngine engine;
    const QString result = engine.getHash(path);

    QVERIFY(result.startsWith(QStringLiteral("smart-")));
}

QTEST_GUILESS_MAIN(TestDupeEngine)
#include "test_engine.moc"
