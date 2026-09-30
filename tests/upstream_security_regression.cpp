#include "playlistimportutil.h"
#include "requestsearchurl.h"
#include "updateversion.h"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <iostream>

namespace {
bool require(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    bool ok = true;
    const auto current = parseUpdateVersion("3.0.1");
    const auto newer = parseUpdateVersion(" 3.0.2\r\n");
    ok &= require(current && newer && *newer > *current, "valid newer version was not accepted");
    ok &= require(parseUpdateVersion("2.99.99") < current, "version comparison is not numeric");
    for (const auto &invalid : {"", "3.0", "3.0.1.2", "3.0.2-beta", "-1.0.0", "+3.0.2",
                               "3. 0.2", "999999999999999.0.0", "99.0.<a href='https://example.com'>x</a>"})
        ok &= require(!parseUpdateVersion(invalid), "malformed update version was accepted");

    for (const auto &text : {QStringLiteral("AC/DC & Friends #1?type=Other"),
                             QString::fromUtf8("Beyonc\xc3\xa9 + 100%"),
                             QStringLiteral("Literal %26 %23 + %2B"), QStringLiteral("")}) {
        const auto url = QUrl::fromEncoded(requestSearchUrl(text).toEncoded());
        const QUrlQuery query(url);
        ok &= require(url.scheme() == "https" && url.host() == "db.openkj.org" && !url.hasFragment(),
                      "search text changed the URL destination or fragment");
        ok &= require(query.queryItems().size() == 2 && query.queryItemValue("type") == "All" &&
                      query.queryItemValue("searchstr", QUrl::FullyDecoded) == text,
                      "search text did not survive URL encoding as a single parameter");
    }

    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!require(db.open(), "could not open test database"))
        return 1;
    QSqlQuery query(db);
    if (!require(query.exec("CREATE TABLE bmsongs (artist TEXT,title TEXT,path TEXT UNIQUE,"
                            "filename TEXT,duration INTEGER,searchstring TEXT)"), "could not create test table"))
        return 1;
    const QString artist = QStringLiteral("O'Connor & \"Friends\"");
    const QString title = QStringLiteral("title\"); DROP TABLE bmsongs; --");
    const QString filename = QString::fromUtf8("Beyonc\xc3\xa9's mix.mp3");
    const QString path = QStringLiteral("C:/Music/") + filename;
    ok &= require(insertPlaylistSong(query, artist, title, path, filename, 213),
                  "quoted playlist metadata could not be imported");
    ok &= require(insertPlaylistSong(query, "replacement", "replacement", path, filename, 1),
                  "duplicate playlist path was not ignored");
    ok &= require(query.exec("SELECT artist,title,path,filename,duration,searchstring FROM bmsongs") && query.next(),
                  "playlist table or imported row was lost");
    ok &= require(query.value(0).toString() == artist && query.value(1).toString() == title &&
                  query.value(2).toString() == path && query.value(3).toString() == filename &&
                  query.value(4).toInt() == 213 &&
                  query.value(5).toString() == artist + " " + title + " " + filename && !query.next(),
                  "playlist values changed or duplicate rows were added");
    query.finish();
    ok &= require(db.transaction(), "could not begin import transaction");
    ok &= require(insertPlaylistSong(query, "relative", "song", "album/track.mp3", "track.mp3", 180),
                  "relative playlist path could not be imported");
    ok &= require(db.rollback(), "could not roll back import transaction");
    ok &= require(query.exec("SELECT COUNT(*) FROM bmsongs") && query.next() && query.value(0).toInt() == 1,
                  "playlist import escaped its transaction");

    if (ok)
        std::cout << "PASS: playlist metadata, update versions and web-search URLs\n";
    return ok ? 0 : 1;
}
