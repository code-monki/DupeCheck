#include "DupeApp.h"

#include <QtConcurrent/QtConcurrent>
#include <QDesktopServices>
#include <QFuture>
#include <QUrl>

#if defined(Q_OS_MACOS)
#  include <QProcess>
#endif

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

DupeApp::DupeApp(QObject* parent)
    : QObject(parent)
    , m_engine(new DupeEngine(this))
    , m_repo(new DataRepository(QStringLiteral("dupecheck.db"), this))
{}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

QStringList DupeApp::scanPaths() const  { return m_scanPaths; }
void DupeApp::setScanPaths(const QStringList& paths) { m_scanPaths = paths; }

QSet<QString> DupeApp::smartExtensions() const
{
    return m_engine->smartExtensions();
}

void DupeApp::setSmartExtensions(const QSet<QString>& extensions)
{
    m_engine->setSmartExtensions(extensions);
}

// ---------------------------------------------------------------------------
// Public actions
// ---------------------------------------------------------------------------

void DupeApp::initiateScan()
{
    if (m_isScanning) {
        emit statusMessage(QStringLiteral("Scan already in progress."));
        return;
    }
    if (m_scanPaths.isEmpty()) {
        emit statusMessage(QStringLiteral("Error: No paths configured."));
        return;
    }

    m_isScanning = true;
    emit scanStarted();

    // Fire-and-forget on Qt's global thread pool.
    QtConcurrent::run([this] { runScan(); });
}

void DupeApp::trashItems(const QStringList& paths)
{
    QStringList trashed;
    for (const QString& path : paths) {
#if defined(Q_OS_MACOS)
        // Use the macOS Finder Trash via osascript so the Undo flow matches
        // user expectations and original file metadata is preserved.
        QProcess p;
        p.start(QStringLiteral("osascript"),
                {QStringLiteral("-e"),
                 QStringLiteral("tell application \"Finder\" to delete POSIX file \"%1\"")
                     .arg(path)});
        if (p.waitForFinished(10'000) && p.exitCode() == 0) {
            trashed.append(path);
        } else {
            emit statusMessage(QStringLiteral("Trash failed: %1").arg(path));
        }
#else
        // Cross-platform fallback: move to the system Trash via QDesktopServices.
        // Qt 6.5+ exposes QFile::moveToTrash() which we prefer.
        if (QFile::moveToTrash(path)) {
            trashed.append(path);
        } else {
            emit statusMessage(QStringLiteral("Trash failed: %1").arg(path));
        }
#endif
    }

    if (!trashed.isEmpty()) {
        m_repo->recordAndDelete(trashed);
        emit trashCompleted(trashed);
    }
}

void DupeApp::undoLastTrash()
{
    const QString restored = m_repo->undoLastTrash();
    if (restored.isEmpty()) {
        emit statusMessage(QStringLiteral("Nothing to undo."));
    } else {
        const QList<FileRecord> dupes = m_repo->getDuplicates();
        emit undoCompleted(restored);
        emit scanFinished(dupes); // reuse the same signal to refresh the tree
        emit statusMessage(QStringLiteral("Restored record: %1").arg(restored));
    }
}

void DupeApp::clearCache()
{
    m_repo->clearAll();
    emit cacheCleared();
    emit statusMessage(QStringLiteral("Database cache cleared."));
}

// ---------------------------------------------------------------------------
// Private — scan worker (runs on QtConcurrent thread)
// ---------------------------------------------------------------------------

void DupeApp::runScan()
{
    QList<FileRecord> batch;

    try {
        for (const QString& targetPath : m_scanPaths) {
            // Report which root we're entering.
            QMetaObject::invokeMethod(this, [this, targetPath] {
                emit scanProgress(0, targetPath);
            }, Qt::QueuedConnection);

            // Connect progress signal for this run.
            // DupeEngine::scanDirectory() emits fileDiscovered() synchronously
            // on this worker thread — collect into the batch.
            QObject ctx; // lifetime guard for the temporary connection
            connect(m_engine, &DupeEngine::fileDiscovered,
                    &ctx,     [&batch](const FileRecord& rec) {
                        batch.append(rec);
                    });
            connect(m_engine, &DupeEngine::progress,
                    &ctx,     [this, &targetPath](int n) {
                        QMetaObject::invokeMethod(this, [this, n, targetPath] {
                            emit scanProgress(n, targetPath);
                        }, Qt::QueuedConnection);
                    });

            m_engine->scanDirectory(targetPath);
        }

        m_repo->upsertRecords(batch);
        const QList<FileRecord> dupes = m_repo->getDuplicates();

        QMetaObject::invokeMethod(this, [this, dupes] {
            emit scanFinished(dupes);
            emit statusMessage(QStringLiteral("Analysis complete. %1 duplicate records found.")
                                   .arg(dupes.size()));
        }, Qt::QueuedConnection);

    } catch (const std::exception& ex) {
        const QString msg = QString::fromUtf8(ex.what());
        QMetaObject::invokeMethod(this, [this, msg] {
            emit scanError(msg);
        }, Qt::QueuedConnection);
    }

    m_isScanning = false;
}
