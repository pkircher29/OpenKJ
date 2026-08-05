#ifndef ARCHIVEPATHUTIL_H
#define ARCHIVEPATHUTIL_H

#include <QDir>
#include <QFileInfo>
#include <QString>

namespace okj {

inline QString archiveEntryStem(const QString &entry)
{
    const QFileInfo info(QDir::fromNativeSeparators(QDir::cleanPath(entry)));
    return (QDir::fromNativeSeparators(info.path()) + "/" + info.completeBaseName()).toCaseFolded();
}

inline bool archiveEntriesShareStem(const QString &first, const QString &second)
{
    return archiveEntryStem(first) == archiveEntryStem(second);
}

} // namespace okj

#endif // ARCHIVEPATHUTIL_H
