#ifndef OPENKJ_PLAYLISTIMPORTUTIL_H
#define OPENKJ_PLAYLISTIMPORTUTIL_H

#include <QSqlQuery>
#include <QString>
#include <QVariant>

inline bool insertPlaylistSong(QSqlQuery &query, const QString &artist, const QString &title,
                               const QString &path, const QString &filename, qint64 duration)
{
    query.prepare("INSERT OR IGNORE INTO bmsongs (artist,title,path,filename,duration,searchstring) "
                  "VALUES(:artist,:title,:path,:filename,:duration,:searchstring)");
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":path", path);
    query.bindValue(":filename", filename);
    query.bindValue(":duration", duration);
    query.bindValue(":searchstring", artist + " " + title + " " + filename);
    return query.exec();
}

#endif
