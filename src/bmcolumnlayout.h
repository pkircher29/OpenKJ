/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef BMCOLUMNLAYOUT_H
#define BMCOLUMNLAYOUT_H

#include <QFontMetrics>
#include <QHeaderView>
#include <QString>
#include <QTableView>
#include <algorithm>
#include <vector>

namespace okj {

// Keep these in sync with TableModelBreakSongs / TableModelPlaylistSongs.
struct BmDbCol {
    static constexpr int id = 0;
    static constexpr int artist = 1;
    static constexpr int title = 2;
    static constexpr int filename = 3;
    static constexpr int duration = 4;
    static constexpr int count = 5;
};

struct BmPlaylistCol {
    static constexpr int id = 0;
    static constexpr int position = 1;
    static constexpr int artist = 2;
    static constexpr int title = 3;
    static constexpr int filename = 4;
    static constexpr int duration = 5;
    static constexpr int path = 6;
    static constexpr int count = 7;
};

struct BmColumnWidths {
    int artist{0};
    int title{0};
    int filename{0};
    int duration{0};
    int icon{0};
};

struct SavedHeaderSection {
    int index{0};
    int size{0};
    bool hidden{false};
    bool indexOk{false};
};

inline int breakMusicTextColumnMinimum(const QFontMetrics &fm)
{
    return std::max(fm.height() * 2, fm.horizontalAdvance(QStringLiteral("Artist")));
}

inline int breakMusicDurationColumnWidth(const QFontMetrics &fm)
{
    return fm.horizontalAdvance(QStringLiteral("Duration")) + fm.height();
}

inline int breakMusicIconColumnWidth(const QFontMetrics &fm)
{
    return std::max(1, fm.height() * 2);
}

inline int breakMusicSectionFloor(const QFont &font)
{
    const QFontMetrics fm(font);
    const int textMin = breakMusicTextColumnMinimum(fm);
    const int duration = breakMusicDurationColumnWidth(fm);
    const int icon = breakMusicIconColumnWidth(fm);
    return std::max(1, std::min(icon, std::min(duration, textMin)));
}

// viewportWidth <= 0, or too narrow to share, still yields a readable width for
// every track column. Callers used to compute
//   (int)((float)(width - duration - 15) * .5)
// which truncates to 0 when the view is not laid out yet. QHeaderView::resizeSection(0)
// is not raised to minimumSectionSize (only sizes > 0 are), so Artist/Title/Filename
// collapsed and only Duration stayed visible.
inline BmColumnWidths breakMusicColumnWidths(int viewportWidth, bool showMetadata, bool showFilenames,
                                             int durationColSize, int iconWidth, int textMin, bool playlist)
{
    durationColSize = std::max(1, durationColSize);
    iconWidth = std::max(1, iconWidth);
    textMin = std::max(1, textMin);

    const int flexCount = (showMetadata ? 2 : 0) + (showFilenames ? 1 : 0);
    const int fixed = durationColSize + (playlist ? iconWidth * 2 : 0);
    constexpr int padding = 15;
    const int minimumTotal = fixed + flexCount * textMin + padding;

    int remaining = flexCount * textMin;
    if (viewportWidth >= minimumTotal)
        remaining = viewportWidth - fixed - padding;

    const auto share = [&](float fraction) {
        const int size = static_cast<int>(static_cast<float>(remaining) * fraction);
        return std::max(textMin, size);
    };

    BmColumnWidths widths;
    widths.duration = durationColSize;
    widths.icon = iconWidth;
    if (showMetadata && showFilenames) {
        widths.artist = share(0.25f);
        widths.title = share(0.25f);
        widths.filename = share(0.50f);
    } else if (showMetadata) {
        widths.artist = share(0.50f);
        widths.title = share(0.50f);
        widths.filename = textMin;
    } else if (showFilenames) {
        widths.artist = textMin;
        widths.title = textMin;
        widths.filename = share(1.0f);
    } else {
        widths.artist = textMin;
        widths.title = textMin;
        widths.filename = textMin;
    }
    return widths;
}

inline void configureBreakMusicHeader(QHeaderView *header, const QFont &font)
{
    if (!header)
        return;
    header->setCascadingSectionResizes(false);
    header->setStretchLastSection(false);
    header->setMinimumSectionSize(breakMusicSectionFloor(font));
}

// Saved per-column state is usable only when it describes each current section
// exactly once and every visible section has a positive width. A mismatched
// count (stale build) or a 0-width visible section is rejected in full.
inline bool savedTableHeaderStateIsUsable(int columnCount, const std::vector<SavedHeaderSection> &sections)
{
    if (columnCount <= 0 || static_cast<int>(sections.size()) != columnCount)
        return false;
    std::vector<bool> seen(static_cast<size_t>(columnCount), false);
    for (const SavedHeaderSection &section : sections) {
        if (!section.indexOk || section.index < 0 || section.index >= columnCount)
            return false;
        if (seen[static_cast<size_t>(section.index)])
            return false;
        seen[static_cast<size_t>(section.index)] = true;
        if (!section.hidden && section.size <= 0)
            return false;
    }
    return true;
}

inline bool breakMusicHeaderIsSensible(const QHeaderView *header, bool playlist, const QFont &font)
{
    if (!header)
        return false;
    const QFontMetrics fm(font);
    const int textMin = breakMusicTextColumnMinimum(fm);
    const int duration = breakMusicDurationColumnWidth(fm);
    const int icon = breakMusicIconColumnWidth(fm);
    const auto wideEnough = [&](int section, int minimum) {
        if (section < 0 || section >= header->count())
            return false;
        if (header->isSectionHidden(section))
            return true;
        return header->sectionSize(section) >= minimum;
    };
    if (playlist) {
        return wideEnough(BmPlaylistCol::artist, textMin)
               && wideEnough(BmPlaylistCol::title, textMin)
               && wideEnough(BmPlaylistCol::filename, textMin)
               && wideEnough(BmPlaylistCol::duration, duration)
               && wideEnough(BmPlaylistCol::id, icon)
               && wideEnough(BmPlaylistCol::path, icon);
    }
    return wideEnough(BmDbCol::artist, textMin)
           && wideEnough(BmDbCol::title, textMin)
           && wideEnough(BmDbCol::filename, textMin)
           && wideEnough(BmDbCol::duration, duration);
}

// Returns true when the view was visible and its columns were redistributed to
// the current viewport. A hidden or not-yet-laid-out view only has collapsed
// sections repaired, so a later show can still do a real layout.
inline bool applyBreakMusicColumns(QTableView *view, bool playlist, bool showMetadata, bool showFilenames,
                                   const QFont &font)
{
    if (!view || !view->horizontalHeader())
        return false;
    QHeaderView *header = view->horizontalHeader();
    const int needed = playlist ? BmPlaylistCol::count : BmDbCol::count;
    if (header->count() < needed)
        return false;

    configureBreakMusicHeader(header, font);

    const QFontMetrics fm(font);
    const int textMin = breakMusicTextColumnMinimum(fm);
    const int duration = breakMusicDurationColumnWidth(fm);
    const int icon = breakMusicIconColumnWidth(fm);
    const int floor = breakMusicSectionFloor(font);
    const int artistCol = playlist ? BmPlaylistCol::artist : BmDbCol::artist;
    const int titleCol = playlist ? BmPlaylistCol::title : BmDbCol::title;
    const int filenameCol = playlist ? BmPlaylistCol::filename : BmDbCol::filename;
    const int durationCol = playlist ? BmPlaylistCol::duration : BmDbCol::duration;

    header->setSectionResizeMode(artistCol, QHeaderView::Interactive);
    header->setSectionResizeMode(titleCol, QHeaderView::Interactive);
    header->setSectionResizeMode(filenameCol, QHeaderView::Interactive);
    header->setSectionResizeMode(durationCol, QHeaderView::Fixed);
    if (playlist) {
        header->setSectionResizeMode(BmPlaylistCol::id, QHeaderView::Fixed);
        header->setSectionResizeMode(BmPlaylistCol::path, QHeaderView::Fixed);
    }

    const bool distribute = view->isVisible() && view->viewport()->width() > 0;
    const BmColumnWidths widths = breakMusicColumnWidths(distribute ? view->viewport()->width() : 0,
                                                         showMetadata, showFilenames, duration, icon, textMin,
                                                         playlist);

    const auto assign = [&](int section, int size) {
        if (section < 0 || section >= header->count())
            return;
        const int desired = std::max(floor, size);
        if (distribute || header->isSectionHidden(section) || header->sectionSize(section) < desired)
            header->resizeSection(section, desired);
    };
    assign(artistCol, widths.artist);
    assign(titleCol, widths.title);
    assign(filenameCol, widths.filename);
    assign(durationCol, widths.duration);
    if (playlist) {
        assign(BmPlaylistCol::id, widths.icon);
        assign(BmPlaylistCol::path, widths.icon);
    }
    return distribute;
}

} // namespace okj

#endif // BMCOLUMNLAYOUT_H
