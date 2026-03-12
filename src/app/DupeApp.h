#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "engine/DataRepository.h"
#include "engine/DupeEngine.h"
#include "types.h"

// ---------------------------------------------------------------------------
// DupeApp — Controller (MVC).
//
// Owns the engine and repository; exposes a clean signal/slot API that the
// View layer connects to. Heavy I/O runs on a QThreadPool worker thread via
// QtConcurrent so the GUI thread is never blocked.
// ---------------------------------------------------------------------------
class DupeApp : public QObject
{
    Q_OBJECT

public:
    explicit DupeApp(QObject* parent = nullptr);

    // --- Configuration accessors (called by ConfigDialog) -----------------
    QStringList scanPaths() const;
    void setScanPaths(const QStringList& paths);

    QSet<QString> smartExtensions() const;
    void setSmartExtensions(const QSet<QString>& extensions);

    // --- Actions (called by MainWindow) -----------------------------------
    void initiateScan();
    void trashItems(const QStringList& paths);
    void undoLastTrash();
    void clearCache();

signals:
    // Emitted on the GUI thread — connect directly to View slots.
    void scanStarted();
    void scanProgress(int filesHashed, const QString& currentPath);
    void scanFinished(const QList<FileRecord>& duplicates);
    void scanError(const QString& message);

    void trashCompleted(const QStringList& trashedPaths);
    void undoCompleted(const QString& restoredPath);
    void cacheCleared();

    void statusMessage(const QString& message);

private:
    DupeEngine*      m_engine;
    DataRepository*  m_repo;
    QStringList      m_scanPaths;
    bool             m_isScanning = false;

    // Worker — runs on QtConcurrent thread pool
    void runScan();
};
