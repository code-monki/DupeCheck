#pragma once

#include <QObject>
#include <QSet>
#include <QString>

#include "types.h"

// ---------------------------------------------------------------------------
// DupeEngine — pure logic layer; no Qt event loop dependency.
// Thread-safe: all methods are stateless or read-only after construction.
// ---------------------------------------------------------------------------
class DupeEngine : public QObject
{
    Q_OBJECT

public:
    explicit DupeEngine(QObject* parent = nullptr);

    // --- Configuration (set before calling scanDirectory) -----------------
    void setSmartExtensions(const QSet<QString>& extensions);
    QSet<QString> smartExtensions() const;

    // --- Public API -------------------------------------------------------

    /// Compute the appropriate hash for a single file. Returns an empty
    /// QString on permission/IO error.
    QString getHash(const QString& filePath) const;

    /// Recursive directory walk. Yields FileRecord objects with hash, size,
    /// and path populated. Emits fileDiscovered() for each valid file;
    /// caller typically runs this on a worker thread.
    void scanDirectory(const QString& rootPath) const;

signals:
    /// Emitted for every processable file found during scanDirectory().
    void fileDiscovered(const FileRecord& record);

    /// Progress report: emitted periodically with the number of files hashed
    /// so far (best-effort; no guarantee of cadence).
    void progress(int filesHashed);

private:
    // --- Hashing internals ------------------------------------------------
    static constexpr qint64 kBlockSize  = 1'048'576; // 1 MB
    static constexpr qint64 kSmartThreshold = kBlockSize * 2; // 2 MB

    QString fullHash(const QString& filePath) const;
    QString smartHash(const QString& filePath) const;

    QSet<QString> m_smartExtensions;
};
