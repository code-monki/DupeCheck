#include "DupeEngine.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

// ---------------------------------------------------------------------------
// Construction / configuration
// ---------------------------------------------------------------------------

DupeEngine::DupeEngine(QObject* parent)
    : QObject(parent)
    , m_smartExtensions{
          QStringLiteral(".mkv"), QStringLiteral(".mp4"),
          QStringLiteral(".iso"), QStringLiteral(".zip"),
          QStringLiteral(".tar"), QStringLiteral(".mov")
      }
{}

void DupeEngine::setSmartExtensions(const QSet<QString>& extensions)
{
    m_smartExtensions = extensions;
}

QSet<QString> DupeEngine::smartExtensions() const
{
    return m_smartExtensions;
}

// ---------------------------------------------------------------------------
// Public scan API
// ---------------------------------------------------------------------------

QString DupeEngine::getHash(const QString& filePath) const
{
    QFileInfo fi(filePath);
    if (m_smartExtensions.contains(fi.suffix().toLower().prepend(QLatin1Char('.'))))
        return smartHash(filePath);
    return fullHash(filePath);
}

void DupeEngine::scanDirectory(const QString& rootPath) const
{
    QDirIterator it(rootPath,
                    QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);

    int count = 0;
    while (it.hasNext()) {
        const QString filePath = it.next();
        const QFileInfo fi(filePath);

        // Skip zero-byte files — they cannot be duplicates.
        if (fi.size() == 0)
            continue;

        // Skip hidden files and any file inside a hidden directory.
        bool hidden = false;
        const QStringList parts = fi.absoluteFilePath().split(QLatin1Char('/'));
        for (const QString& part : parts) {
            if (!part.isEmpty() && part.startsWith(QLatin1Char('.'))) {
                hidden = true;
                break;
            }
        }
        if (hidden)
            continue;

        const QString hash = getHash(filePath);
        if (hash.isEmpty())
            continue; // permission error — skip silently

        FileRecord rec;
        rec.hash = hash;
        rec.size = fi.size();
        rec.path = fi.absoluteFilePath();

        emit fileDiscovered(rec);

        if (++count % 100 == 0)
            emit progress(count);
    }
    emit progress(count);
}

// ---------------------------------------------------------------------------
// Private hashing helpers
// ---------------------------------------------------------------------------

QString DupeEngine::fullHash(const QString& filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return {};

    QCryptographicHash hasher(QCryptographicHash::Sha256);
    char buf[kBlockSize];
    while (!f.atEnd()) {
        const qint64 read = f.read(buf, kBlockSize);
        if (read <= 0)
            break;
        hasher.addData(QByteArray::fromRawData(buf, static_cast<int>(read)));
    }
    return QString::fromLatin1(hasher.result().toHex());
}

QString DupeEngine::smartHash(const QString& filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return {};

    const qint64 size = f.size();

    // Fall back to full hash for small files.
    if (size <= kSmartThreshold)
        return fullHash(filePath);

    QCryptographicHash hasher(QCryptographicHash::Sha256);

    // First 1 MB
    {
        const QByteArray head = f.read(kBlockSize);
        if (head.isEmpty())
            return {};
        hasher.addData(head);
    }

    // Last 1 MB
    if (!f.seek(size - kBlockSize))
        return {};
    {
        const QByteArray tail = f.read(kBlockSize);
        if (tail.isEmpty())
            return {};
        hasher.addData(tail);
    }

    // The "smart-" prefix distinguishes this digest from a full SHA-256 so the
    // two hash spaces never collide in the database.
    return QStringLiteral("smart-") + QString::fromLatin1(hasher.result().toHex());
}
