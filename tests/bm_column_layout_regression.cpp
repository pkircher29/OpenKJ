#include "bmcolumnlayout.h"
#include "settings.h"

#include <QApplication>
#include <QFont>
#include <QHeaderView>
#include <QSettings>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTableView>

#include <iostream>

namespace {

bool require(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

QFont testFont()
{
    QFont font;
    font.setPointSize(14);
    return font;
}

void zeroManagedSections(QHeaderView *header)
{
    header->setMinimumSectionSize(0);
    for (int section = 0; section < header->count(); ++section)
        header->resizeSection(section, 0);
}

} // namespace

int main(int argc, char **argv)
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("OpenKJ");

    bool ok = true;
    const QFont font = testFont();
    const QFontMetrics fm(font);
    const int textMin = okj::breakMusicTextColumnMinimum(fm);
    const int duration = okj::breakMusicDurationColumnWidth(fm);
    const int icon = okj::breakMusicIconColumnWidth(fm);

    // The old autosize cast truncates a slightly negative remainder to 0.
    // resizeSection(0) then bypasses QHeaderView's minimum-size clamp.
    const int collapsedRemainder = -1;
    ok &= require(static_cast<int>(static_cast<float>(collapsedRemainder) * 0.5f) == 0,
                  "historical remainder cast no longer truncates to 0");
    const int narrowViewport = duration + 10;
    const int oldArtist = static_cast<int>(static_cast<float>(narrowViewport - duration - 15) * 0.5f);
    ok &= require(oldArtist <= 0, "historical narrow-width artist size was not <= 0");

    const okj::BmColumnWidths narrow = okj::breakMusicColumnWidths(narrowViewport, true, false, duration, icon, textMin,
                                                                   false);
    ok &= require(narrow.artist >= textMin, "narrow viewport collapsed artist");
    ok &= require(narrow.title >= textMin, "narrow viewport collapsed title");
    ok &= require(narrow.filename >= textMin, "narrow viewport stored a 0 filename width");
    ok &= require(narrow.duration >= duration, "narrow viewport collapsed duration");

    const okj::BmColumnWidths unlaidOut = okj::breakMusicColumnWidths(0, true, true, duration, icon, textMin, true);
    ok &= require(unlaidOut.artist >= textMin && unlaidOut.title >= textMin && unlaidOut.filename >= textMin,
                  "unlaid-out viewport collapsed a track column");
    ok &= require(unlaidOut.duration >= duration && unlaidOut.icon >= icon, "unlaid-out viewport collapsed a fixed column");

    const int wideViewport = 1200;
    const okj::BmColumnWidths wide = okj::breakMusicColumnWidths(wideViewport, true, true, duration, icon, textMin, false);
    ok &= require(wide.artist >= textMin && wide.title >= textMin && wide.filename >= textMin,
                  "wide viewport produced a track column below the minimum");
    ok &= require(wide.filename >= wide.artist && wide.filename >= wide.title,
                  "filename did not receive the larger share");

    QStandardItemModel dbModel(1, okj::BmDbCol::count);
    QTableView dbView;
    dbView.setModel(&dbModel);
    dbView.setObjectName("tableViewBmDb");
    QHeaderView *dbHeader = dbView.horizontalHeader();
    dbHeader->setMinimumSectionSize(1);
    dbHeader->resizeSection(okj::BmDbCol::artist, 250);
    dbHeader->resizeSection(okj::BmDbCol::title, 250);
    dbHeader->resizeSection(okj::BmDbCol::filename, 250);
    dbHeader->resizeSection(okj::BmDbCol::duration, duration);
    ok &= require(!okj::applyBreakMusicColumns(&dbView, false, true, true, font),
                  "hidden view was treated as laid out");
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::artist) == 250, "hidden view shrank a sensible artist column");
    ok &= require(!dbHeader->cascadingSectionResizes(), "cascading resize stayed enabled");
    ok &= require(dbHeader->minimumSectionSize() > 0, "minimum section size was left at 0");

    zeroManagedSections(dbHeader);
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::artist) == 0, "test could not collapse a section to 0");
    okj::applyBreakMusicColumns(&dbView, false, true, false, font);
    ok &= require(okj::breakMusicHeaderIsSensible(dbHeader, false, font),
                  "repair left a collapsed break-music database header");

    dbView.resize(wideViewport, 320);
    dbView.show();
    app.processEvents();
    ok &= require(okj::applyBreakMusicColumns(&dbView, false, true, true, font),
                  "visible database view was not distributed");
    ok &= require(okj::breakMusicHeaderIsSensible(dbHeader, false, font),
                  "visible database view still had an unsensible column");
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::filename) >= dbHeader->sectionSize(okj::BmDbCol::artist),
                  "visible database filename column was narrower than artist");

    dbView.resize(narrowViewport, 320);
    app.processEvents();
    ok &= require(okj::applyBreakMusicColumns(&dbView, false, true, false, font),
                  "narrow visible database view was not updated");
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::artist) >= textMin, "narrow visible view collapsed artist");
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::title) >= textMin, "narrow visible view collapsed title");
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::duration) >= duration, "narrow visible view collapsed duration");

    QStandardItemModel playlistModel(1, okj::BmPlaylistCol::count);
    QTableView playlistView;
    playlistView.setModel(&playlistModel);
    playlistView.resize(wideViewport, 320);
    playlistView.show();
    app.processEvents();
    zeroManagedSections(playlistView.horizontalHeader());
    ok &= require(okj::applyBreakMusicColumns(&playlistView, true, true, true, font),
                  "visible playlist view was not distributed");
    ok &= require(okj::breakMusicHeaderIsSensible(playlistView.horizontalHeader(), true, font),
                  "playlist header was not sensible after layout");

    Settings settings;
    auto savedSize = [](const QString &viewName, int section) {
        QSettings raw;
        raw.beginGroup(viewName);
        raw.beginGroup(QString::number(section));
        const int size = raw.value("size", -1).toInt();
        raw.endGroup();
        raw.endGroup();
        return size;
    };

    dbHeader->setMinimumSectionSize(1);
    dbHeader->resizeSection(okj::BmDbCol::artist, 180);
    dbHeader->resizeSection(okj::BmDbCol::title, 160);
    dbHeader->resizeSection(okj::BmDbCol::filename, 220);
    dbHeader->resizeSection(okj::BmDbCol::duration, duration);
    dbHeader->setSectionHidden(okj::BmDbCol::id, true);
    settings.saveColumnWidths(&dbView);
    settings.sync();
    ok &= require(savedSize("tableViewBmDb", okj::BmDbCol::artist) == 180, "valid artist width was not saved");

    dbHeader->resizeSection(okj::BmDbCol::artist, 90);
    dbHeader->setSectionHidden(okj::BmDbCol::id, false);
    ok &= require(settings.restoreColumnWidths(&dbView), "valid header state was rejected");
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::artist) == 180, "valid artist width was not restored");
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::title) == 160, "valid title width was not restored");
    ok &= require(dbHeader->isSectionHidden(okj::BmDbCol::id), "hidden id column was shown by restore");

    dbHeader->setMinimumSectionSize(0);
    dbHeader->setSectionHidden(okj::BmDbCol::artist, false);
    dbHeader->resizeSection(okj::BmDbCol::artist, 0);
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::artist) == 0, "artist column could not be collapsed for the save test");
    settings.saveColumnWidths(&dbView);
    settings.sync();
    ok &= require(!settings.restoreColumnWidths(&dbView), "collapsed visible column was persisted and restored");
    dbHeader->setMinimumSectionSize(1);
    dbHeader->resizeSection(okj::BmDbCol::artist, 140);
    ok &= require(!settings.restoreColumnWidths(&dbView), "missing header state restored a layout");
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::artist) == 140, "rejected restore changed the artist column");

    {
        QSettings raw;
        raw.beginGroup("tableViewBmDb");
        for (int section = 0; section < okj::BmDbCol::count; ++section) {
            raw.beginGroup(QString::number(section));
            raw.setValue("size", section == okj::BmDbCol::artist ? 0 : 120);
            raw.setValue("hidden", false);
            raw.endGroup();
        }
        raw.endGroup();
        raw.sync();
    }
    settings.sync();
    dbHeader->resizeSection(okj::BmDbCol::artist, 155);
    ok &= require(!settings.restoreColumnWidths(&dbView), "zero-width saved artist column was applied");
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::artist) == 155, "invalid header state overwrote artist");

    {
        QSettings raw;
        raw.beginGroup("tableViewBmDb");
        raw.remove("");
        for (int section = 0; section < okj::BmDbCol::count - 1; ++section) {
            raw.beginGroup(QString::number(section));
            raw.setValue("size", 130);
            raw.setValue("hidden", false);
            raw.endGroup();
        }
        raw.beginGroup("stale");
        raw.setValue("size", 130);
        raw.setValue("hidden", false);
        raw.endGroup();
        raw.endGroup();
        raw.sync();
    }
    settings.sync();
    dbHeader->resizeSection(okj::BmDbCol::title, 147);
    ok &= require(!settings.restoreColumnWidths(&dbView), "stale header group was applied");
    ok &= require(dbHeader->sectionSize(okj::BmDbCol::title) == 147, "stale header state overwrote title");

    if (ok)
        std::cout << "PASS: break-music columns stay readable and invalid header state is ignored\n";
    return ok ? 0 : 1;
}
