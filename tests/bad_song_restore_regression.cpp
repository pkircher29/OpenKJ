#include "models/tablemodelkaraokesongs.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <iostream>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

namespace {

bool require(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

bool requireEq(const QString &actual, const QString &expected, const char *message)
{
    if (actual == expected)
        return true;
    std::cerr << "FAIL: " << message << " (expected '" << expected.toStdString() << "' got '" << actual.toStdString()
              << "')\n";
    return false;
}

bool waitForRowCount(TableModelKaraokeSongs &model, int expected)
{
    // Library search applies on a 100ms timer. Always let that timer run, because
    // the previous result can already have the expected row count.
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 300)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    if (model.rowCount({}) == expected)
        return true;
    std::cerr << "FAIL: row count is " << model.rowCount({}) << ", expected " << expected << '\n';
    return false;
}

QString discIdForPath(const QString &path)
{
    QSqlQuery query;
    query.prepare("SELECT discid FROM dbsongs WHERE path = :path");
    query.bindValue(":path", path);
    if (!query.exec() || !query.next())
        return QStringLiteral("<missing>");
    return query.value(0).toString();
}

QString savedDiscIdForPath(const QString &path)
{
    QSqlQuery query;
    query.prepare("SELECT saveddiscid FROM dbsongs WHERE path = :path");
    query.bindValue(":path", path);
    if (!query.exec() || !query.next())
        return QStringLiteral("<missing>");
    if (query.value(0).isNull())
        return QStringLiteral("<null>");
    return query.value(0).toString();
}

bool insertSong(const QString &artist, const QString &title, const QString &discId, const QString &path,
                const QString &filename, const QString &search)
{
    QSqlQuery query;
    query.prepare("INSERT INTO dbsongs (artist, title, discid, duration, path, filename, searchstring, plays) "
                  "VALUES (:artist, :title, :discid, 0, :path, :filename, :search, 0)");
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":discid", discId);
    query.bindValue(":path", path);
    query.bindValue(":filename", filename);
    query.bindValue(":search", search);
    if (query.exec())
        return true;
    std::cerr << "FAIL: insert " << path.toStdString() << ": " << query.lastError().text().toStdString() << '\n';
    return false;
}

const okj::KaraokeSong *findSong(const std::vector<std::shared_ptr<okj::KaraokeSong>> &songs, const QString &path)
{
    for (const auto &song : songs) {
        if (song->path == path)
            return song.get();
    }
    return nullptr;
}

} // namespace

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("OpenKJBadSongRestoreTest");

    auto logger = std::make_shared<spdlog::logger>("logger", std::make_shared<spdlog::sinks::null_sink_mt>());
    spdlog::register_logger(logger);

    bool ok = true;
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(QStringLiteral(":memory:"));
        if (!db.open()) {
            std::cerr << "FAIL: could not open sqlite database: " << db.lastError().text().toStdString() << '\n';
            return 1;
        }

        QSqlQuery schema;
        if (!schema.exec("CREATE TABLE dbsongs ("
                         "songid INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "artist TEXT, title TEXT, discid TEXT, duration INTEGER, "
                         "path VARCHAR(700) NOT NULL UNIQUE, filename TEXT, searchstring TEXT, "
                         "plays INT DEFAULT 0, lastplay TIMESTAMP, saveddiscid TEXT)")) {
            std::cerr << "FAIL: schema: " << schema.lastError().text().toStdString() << '\n';
            return 1;
        }

        const QString goodPath = QStringLiteral("/library/SC1001 - Queen - Good Song.mp4");
        const QString badPath = QStringLiteral("/library/SC 1002 - Queen - Bad Song.mp4");
        const QString legacyPath = QStringLiteral("/library/SC7777 - Abba - Legacy.mp4");
        const QString blankPath = QStringLiteral("/library/NoId - Artist - Blank.mp4");
        const QString lostPath = QStringLiteral("/library/mystery.mp4");

        ok &= insertSong("Queen", "Good Song", "SC1001", goodPath, "SC1001 - Queen - Good Song",
                         "SC1001 - Queen - Good Song Queen Good Song SC1001");
        ok &= insertSong("Queen", "Bad Song", "SC 1002", badPath, "SC 1002 - Queen - Bad Song",
                         "SC 1002 - Queen - Bad Song Queen Bad Song SC 1002");
        ok &= insertSong("Abba", "Legacy", "!!BAD!!", legacyPath, "SC7777 - Abba - Legacy.mp4",
                         "SC7777 - Abba - Legacy Abba Legacy SC7777");
        ok &= insertSong("Artist", "Blank", "", blankPath, "NoId - Artist - Blank",
                         "NoId - Artist - Blank Artist Blank OLD-ID");
        ok &= insertSong("Unknown", "Lost", "!!BAD!!", lostPath, "mystery", "not the usual search text");

        ok &= requireEq(TableModelKaraokeSongs::recoverDiscIdFromSearchString(
                                "SC7777 - Abba - Legacy.mp4", "Abba", "Legacy",
                                "SC7777 - Abba - Legacy Abba Legacy SC7777"),
                        "SC7777", "recover disc id when the filename includes an extension");
        ok &= require(TableModelKaraokeSongs::recoverDiscIdFromSearchString("file", "A", "T", "file A T !!BAD!!").isEmpty(),
                      "do not recover the bad-song marker as a disc id");

        {
            TableModelKaraokeSongs model;
            model.loadData();
            ok &= require(waitForRowCount(model, 3), "songs already marked bad stay out of the library");

            const auto badBefore = model.badSongs();
            ok &= require(badBefore.size() == 2, "only songs flagged bad are listed for restore");
            const auto *legacy = findSong(badBefore, legacyPath);
            const auto *lost = findSong(badBefore, lostPath);
            ok &= require(legacy && legacy->songid == "SC7777", "legacy bad song recovers its disc id from search text");
            ok &= requireEq(savedDiscIdForPath(legacyPath), "SC7777", "recovered disc id is saved for the next launch");
            ok &= require(lost && lost->songid.isEmpty(), "unrecoverable bad song does not keep !!BAD!! as its song id");

            model.search("Bad Song");
            ok &= require(waitForRowCount(model, 1), "song is searchable before it is marked bad");
            model.markSongBad(badPath);
            ok &= require(model.rowCount({}) == 0, "marking a song bad removes it from the current search");
            ok &= requireEq(discIdForPath(badPath), "!!BAD!!", "marked song is stored with the bad marker");
            ok &= requireEq(savedDiscIdForPath(badPath), "SC 1002", "original disc id is kept when a song is marked bad");
            ok &= requireEq(discIdForPath(goodPath), "SC1001", "marking one song bad leaves other songs alone");
            const auto *marked = findSong(model.badSongs(), badPath);
            ok &= require(marked && marked->songid == "SC 1002", "song id stays available in memory after marking bad");

            ok &= require(model.restoreSong(badPath), "restore a song that was just marked bad");
            ok &= require(model.rowCount({}) == 1, "restored song matches the current search again");
            ok &= requireEq(discIdForPath(badPath), "SC 1002", "restore writes the original disc id back");
            ok &= requireEq(savedDiscIdForPath(badPath), "<null>", "restore clears the saved disc id");
            ok &= require(!model.restoreSong(badPath), "restoring a song that is not bad does nothing");
            ok &= require(!model.restoreSong(goodPath), "restoring a normal song does nothing");
            ok &= requireEq(discIdForPath(goodPath), "SC1001", "failed restore does not change a normal song");

            model.search("Blank");
            ok &= require(waitForRowCount(model, 1), "song with an empty disc id is still in the library");
            model.markSongBad(blankPath);
            ok &= require(model.rowCount({}) == 0, "empty disc id can still be marked bad");
            ok &= requireEq(savedDiscIdForPath(blankPath), "", "empty disc id is saved as empty rather than guessed");
            ok &= require(model.restoreSong(blankPath), "restore a song whose disc id was empty");
            ok &= require(model.rowCount({}) == 1, "song with an empty disc id returns to search");
            ok &= requireEq(discIdForPath(blankPath), "", "restore keeps an empty disc id empty");

            model.search("Legacy");
            ok &= require(waitForRowCount(model, 0), "legacy bad song is hidden from search");
            ok &= require(model.restoreSong(legacyPath), "restore a song marked bad by an older version");
            ok &= require(model.rowCount({}) == 1, "legacy song shows up in search after restore");
            ok &= requireEq(discIdForPath(legacyPath), "SC7777", "legacy restore uses the recovered disc id");

            model.search("Lost");
            ok &= require(waitForRowCount(model, 0), "bad song with no recoverable id stays hidden");
            ok &= require(model.restoreSong(lostPath), "restore still unhides a song when its old disc id is unknown");
            ok &= require(model.rowCount({}) == 1, "song with an unknown old disc id returns to the library");
            ok &= requireEq(discIdForPath(lostPath), "", "unknown disc id is cleared instead of left as !!BAD!!");
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);

    if (ok)
        std::cout << "PASS: songs marked bad can be restored without losing the disc id\n";
    return ok ? 0 : 1;
}
