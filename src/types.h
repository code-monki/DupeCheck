#pragma once

#include <QList>
#include <QMetaType>
#include <QString>

// ---------------------------------------------------------------------------
// FileRecord — the canonical data unit passed through the entire MVC stack.
// Used in signals that cross thread boundaries, so it is registered as a
// Qt meta-type in main.cpp.
// ---------------------------------------------------------------------------
struct FileRecord {
    QString hash;
    qint64  size     = 0;
    QString path;
    QString lastSeen; // ISO-8601 datetime; populated from SQLite datetime('now')
};

Q_DECLARE_METATYPE(FileRecord)
Q_DECLARE_METATYPE(QList<FileRecord>)
