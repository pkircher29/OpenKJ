#include "models/tablemodelkaraokesongs.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <functional>
#include <iostream>

namespace {

bool require(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

bool waitUntil(const std::function<bool()> &ready, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (!ready()) {
        if (timer.elapsed() > timeoutMs)
            return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    return true;
}

int rowForSongId(const TableModelKaraokeSongs &model, int songId)
{
    for (int row = 0; row < model.rowCount(QModelIndex()); ++row) {
        if (model.data(model.index(row, TableModelKaraokeSongs::COL_ID), Qt::DisplayRole).toInt() == songId)
            return row;
    }
    return -1;
}

bool insertSong(int songId, const QString &artist, const QString &title, const QString &discId)
{
    const QString filename = QString("song%1.mp4").arg(songId);
    const QString path = QString("/library/%1").arg(filename);
    const QString searchString =
            (QFileInfo(path).completeBaseName() + " " + artist + " " + title + " " + discId).toLower();
    QSqlQuery query;
    query.prepare("INSERT INTO dbsongs (songid, artist, title, discid, duration, filename, path, searchstring, plays, lastplay) "
                  "VALUES (:songid, :artist, :title, :discid, 0, :filename, :path, :searchstring, 0, '')");
    query.bindValue(":songid", songId);
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":discid", discId);
    query.bindValue(":filename", filename);
    query.bindValue(":path", path);
    query.bindValue(":searchstring", searchString);
    if (!query.exec()) {
        std::cerr << "FAIL: insert song: " << query.lastError().text().toStdString() << '\n';
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QTemporaryDir settingsDir;
    if (!require(settingsDir.isValid(), "temporary settings directory"))
        return 1;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());

    auto logger = spdlog::stdout_color_mt("logger");
    logger->set_level(spdlog::level::warn);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!require(db.open(), "open in-memory song database"))
        return 1;
    QSqlQuery query;
    if (!require(query.exec(
            "CREATE TABLE dbsongs ("
            "songid INTEGER PRIMARY KEY,"
            "artist TEXT,"
            "title TEXT,"
            "discid TEXT,"
            "duration INTEGER,"
            "filename TEXT,"
            "path TEXT,"
            "searchstring TEXT,"
            "plays INTEGER,"
            "lastplay TEXT)"), "create dbsongs"))
        return 1;

    if (!insertSong(1, "Plain One", "First", "SC1")
        || !insertSong(2, "Plain Two", "Second", "SC2")
        || !insertSong(3, "Keepme One", "Third", "SC3")
        || !insertSong(4, "Keepme Two", "Fourth", "SC4")
        || !insertSong(5, "Plain Five", "Fifth", "SC5"))
        return 1;

    TableModelKaraokeSongs model;
    model.loadData();
    if (!require(waitUntil([&model]() { return model.rowCount(QModelIndex()) == 5; }, 2000),
                 "library load populates every song"))
        return 1;

    const int originalRow = rowForSongId(model, 3);
    if (!require(originalRow >= 0, "song 3 is visible before the edit"))
        return 1;

    const int keptRow = model.updateEditedSong(
            3, "Keepme One", "Retitled", "SC3", "song3.mp4", "/library/song3.mp4");
    if (!require(keptRow == originalRow, "edited song stays on its current row"))
        return 1;
    if (!require(model.rowCount(QModelIndex()) == 5, "in-place edit does not reload the library"))
        return 1;
    if (!require(model.data(model.index(keptRow, TableModelKaraokeSongs::COL_ID), Qt::DisplayRole).toInt() == 3,
                 "selected row is still the edited song"))
        return 1;
    if (!require(model.data(model.index(keptRow, TableModelKaraokeSongs::COL_TITLE), Qt::DisplayRole).toString() == "Retitled",
                 "edited title is shown on the selected row"))
        return 1;
    if (!require(model.data(model.index(keptRow, TableModelKaraokeSongs::COL_ARTIST), Qt::DisplayRole).toString() == "Keepme One",
                 "edited artist is shown on the selected row"))
        return 1;

    model.search("retitled");
    if (!require(waitUntil([&model]() { return model.rowCount(QModelIndex()) == 1; }, 2000),
                 "updated search text finds the edited song"))
        return 1;
    if (!require(rowForSongId(model, 3) == 0, "edited song remains selectable after search"))
        return 1;

    model.search("keepme");
    if (!require(waitUntil([&model]() { return model.rowCount(QModelIndex()) == 2; }, 2000),
                 "search narrows to the two keepme songs"))
        return 1;

    const int removedSelection = model.updateEditedSong(
            3, "Other Artist", "Other Title", "SC3", "song3.mp4", "/library/song3.mp4");
    if (!require(model.rowCount(QModelIndex()) == 1, "song that leaves the search is removed"))
        return 1;
    if (!require(removedSelection == 0, "selection stays on the next visible library row"))
        return 1;
    if (!require(model.data(model.index(removedSelection, TableModelKaraokeSongs::COL_ID), Qt::DisplayRole).toInt() == 4,
                 "the following song is the one left selected"))
        return 1;

    if (!require(model.updateEditedSong(99, "Missing", "Missing", "SC99", "missing.mp4", "/library/missing.mp4") == -1,
                 "unknown song id does not invent a selection"))
        return 1;

    std::cout << "PASS\n";
    return 0;
}
