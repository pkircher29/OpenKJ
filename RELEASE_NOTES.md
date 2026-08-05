# OpenKJ 3.0.0 — Reliability & Performance Audit Release

**Audit date:** August 5, 2026
**Scope:** Windows, Linux, and macOS builds; playback/rendering; database and import paths; metadata handling; SongShop networking; Qt models; concurrency; and build configuration.

## Community release and availability

OpenKJ 3.0 is a free, community-maintained maintenance release intended to give existing OpenKJ hosts a stable, audited build path. It does not claim to replace or represent upstream OpenKJ support, services, or releases.

- **Free OpenKJ 3.0:** this bug-fix and maintenance release remains open source and free of charge.
- **Optional hosted alternative:** OpenKJ-compatible web hosting is offered separately for hosts who want online songbooks and requests; it is an alternative service, not a claim about the availability of upstream hosting.
- **Optional modern migration path:** [Auto-KJ](https://auto-kj.com/#pricing) is a separate platform with a 60-day trial and an OpenKJ singer-history/song-database import path. Hosts can also continue using OpenKJ or use Auto-KJ's offline free tier; there are no forced upgrades.
- **Release link:** the verified GitHub Release URL will be inserted after the signed/tagged publication has completed.

The full public-facing announcement is maintained in [COMMUNITY_ANNOUNCEMENT.md](COMMUNITY_ANNOUNCEMENT.md).

## Highlights

This release focuses on crash prevention, playback correctness, database safety, predictable network failure handling, and measurable songbook-generation performance.

- Fixed a confirmed GStreamer double-unref during media backend teardown.
- Fixed a second confirmed GStreamer message double-unref in audio recording and wait for EOS so encoded recordings finalize cleanly.
- Fixed undefined behavior in CD+G scrolling and removed its large per-call stack buffers.
- Fixed singer-history refresh logic that cleared valid history instead of loading it.
- Fixed a possible background-directory-scan use-after-free.
- Fixed uninitialized software-video image formats and rejected unsupported or malformed frames safely.
- Replaced songbook PDF generation's N+1 database access with one ordered query.
- Converted user-controlled SQL operations to prepared statements with bound parameters.
- Prevented SongShop login failures and malformed replies from trapping the UI in an endless event loop.
- Fixed songbook uploads dropping rows at every 1,000-song boundary.
- Disabled and purged legacy card/CVV persistence.

## Playback, GStreamer, and video fixes

- Corrected `MediaBackend` teardown to release each explicitly retained audio/video bin exactly once.
  - A GStreamer reference-count probe measured one remaining reference after pipeline removal.
  - The former second unref reproduces a fatal GStreamer `ref_count > 0` assertion.
- Clarified and corrected ownership in `gsthlp_get_peer_element()`.
  - The helper now returns an owned reference.
  - Audio and video callers release that reference after use.
- Added null checks for missing pads, missing peers, and missing parent elements in GStreamer helper code.
- Made pad enumeration ignore pads with no parent element instead of returning invalid entries.
- Hardened the software-rendered video sink:
  - validates samples, caps, structures, dimensions, pixel formats, buffers, and buffer mapping;
  - initializes the Qt image format explicitly;
  - accepts the supported `RGB16` and `BGRx` formats only;
  - rejects unsupported formats rather than constructing a `QImage` with an uninitialized format;
  - cleans up samples and mapping state correctly on all failure paths.
- Annotated the intentional GStreamer message fallthrough so static analysis and maintainers do not mistake it for a missing `break`.

## CD+G rendering fixes

- Replaced incorrectly typed scratch buffers (`unsigned char*[]`) with actual byte storage.
- Replaced overlapping `memcpy()` calls with `memmove()` for horizontal and vertical scroll operations.
- Removed approximately 57 KB of per-call stack allocation reported by MSVC analysis.
- Added a reusable scroll scratch buffer allocated once per `CdgImageFrame`, avoiding heap allocation during each real-time scroll command.
- Preserved copy-scroll behavior in all four directions: left, right, up, and down.

## Database and SQL fixes

- Replaced string-built custom-pattern SQL with prepared statements and bound values for insert, update, and delete operations.
- Added SQL failure logging for custom-pattern changes.
- Replaced string-built import lookup queries with prepared statements for exact and `LIKE` searches.
- Replaced string-built custom-pattern lookup in the database dialog with a prepared statement.
- Fixed custom pattern names containing `: ` so the complete name is retained when selected.
- Replaced heap-allocated `QFile` objects in legacy import helpers with stack-owned files.
- Verified quoted values such as `O'Brien: Custom` round-trip through the prepared SQL paths.
- Guarded moved-file matching when `std::lower_bound()` returns the end iterator.
- Wrapped break-music replacement in a checked transaction so failed imports roll back instead of leaving a partial database.

## Songbook API and archive correctness

- Repaired songbook-upload cursor iteration so every row is sent exactly once, including 1,000-row boundaries and exact multiples.
- Avoided empty trailing upload documents and added SQL/network failure handling and reply cleanup.
- Validated reply payloads before accepting server-generated book/request identifiers.
- Required ZIP audio and CD+G entries to share the same basename instead of silently pairing unrelated files.
- Reset archive scan state between files and rejected failed or empty archive listings safely.

## Singer-history and Qt model fixes

- Fixed `TableModelHistorySongs::refresh()` using the singer-existence condition backwards.
  - Existing singers now reload their history.
  - Missing singers now clear the model.
- Replaced invalid insert/layout notifications in history loading with `beginResetModel()` / `endResetModel()`.
- Sorting now compares history-song objects by const reference instead of copying each object repeatedly.
- Fixed `ForegroundRole` in the rotation model falling through into `DisplayRole`.
  - Non-current rows no longer return text when Qt requests a foreground brush/color.
- Added explicit breaks for harmless but ambiguous switch fallthroughs.
- Added explicit integer conversions at Qt model boundaries to remove narrowing ambiguity.
- Avoided an unnecessary `QModelIndex` copy in history playback.

## Songbook generation performance

- Replaced the previous artist query plus one title query per artist with one ordered `SELECT DISTINCT artist, title` query.
- Preserved artist/title grouping and ordering, including empty-artist edge cases.
- Synthetic benchmark at 100,000 songs and 5,000 artists:
  - old path: 5,001 SQL queries;
  - new path: 1 SQL query;
  - measured median SQL-stage improvement: **13.2%**.

## Directory monitoring and concurrency

- Removed the `DirectoryMonitor` object pointer from its background `QtConcurrent::run()` task.
- Made path enumeration static because it does not use instance state.
- Prevents a worker from invoking a member function through a destroyed `DirectoryMonitor` when a dialog is closed during a large directory scan.

## Metadata and tag-reading fixes

- Reset artist, title, disc ID, duration, and track number before reading each new file so absent tags cannot leak metadata from the previously inspected song.
- Handle GStreamer discoverer creation failure safely and log the underlying error.
- Guard discoverer teardown and media reads when initialization failed.
- Free strings returned by GStreamer tag APIs after copying them into Qt strings.
- Guard TagLib tag and audio-property pointers before dereferencing them.
- Preserve the TagLib fallback for media not handled by GStreamer discovery.

## SongShop reliability and networking

- Tagged network replies by request purpose (`catalog`, `login`, or `purchase`) so failures update the correct state.
- Fixed network login errors leaving `knLoginError` false and trapping the purchase UI in an endless event-processing loop.
- Treat malformed or unexpected login/purchase JSON as explicit failure instead of leaving requests unresolved.
- Added 30-second transfer timeouts for login and purchase requests on supported Qt versions.
- Added a 120-second transfer timeout for paid song downloads.
- Sanitize downloaded filenames, create nested download directories safely, and prevent server-provided names from escaping the configured destination.
- Save downloaded songs atomically with `QSaveFile` and no longer report success after network, file-open, short-write, or commit failures.
- Schedule completed `QNetworkReply` objects for deletion on every response path.
- Build request URLs with `QUrlQuery` so usernames, hashes, song IDs, and other values are encoded correctly.
- Hash the UTF-8 password bytes directly, fixing incorrect handling of non-ASCII passwords.
- Prevent corrupt or incompatible saved-card data from crashing through out-of-range `QStringList::at()` calls.
- Removed saved-card/CVV persistence entirely and purge legacy values at startup.
- Fixed the purchase progress dialog leak.
- Restored purchase-dialog event handling when password entry is cancelled during setup.

## Build-system and defensive initialization changes

- Made the GStreamer SDK location configurable through the `GST_BASE_PATH` CMake cache variable.
- Added an early configure-time failure with an actionable message when GStreamer headers or libraries are missing.
- Removed a hard-coded machine-specific GStreamer path from the build logic.
- Added safe defaults for previously uninitialized fields in:
  - karaoke song play counts;
  - ZIP entry sizes;
  - songbook request IDs, key changes, times, venue IDs, and accepting state;
  - SongShop song type and price;
  - karaoke filename pattern selection;
  - break-song duration;
  - GStreamer pad element pointers.
- Removed compiler-warning-only IDE pragmas and clarified narrowing conversions.

## Verification performed

- Windows x64 Release build: **PASS**.
- Final artifact: `build-fixed/Release/openkj.exe`.
- `git diff --check`: **PASS**.
- Full MSVC native code-analysis pass: **completed successfully** and used to identify the rendering, stack-use, initialization, fallthrough, and copy issues fixed above.
- CTest regression suite: **2/2 PASS** in Release for CD+G scroll/bounds handling and payment-setting migration/security.
- CD+G executable regression probe: **PASS** for left, right, up, and down copy scrolling.
- GStreamer ownership probe:
  - corrected one-unref path: exit 0;
  - former double-unref path: fatal GStreamer assertion, confirming the bug.
- SQL regression checks: **PASS** for quoted custom-pattern values and old/new songbook output equivalence.
- CMake GStreamer path validation: **PASS**, including expected failure for an invalid SDK path.
- Native application startup smoke with isolated settings: process remained alive without immediate GLib/GStreamer critical errors.

## Known issues not resolved in this release

### Legacy SongShop payment transport

OpenKJ 3.0 no longer stores card numbers or CVV values. The legacy saved-card setting is purged when encountered, the save-card control is disabled, and payment fields must be entered for each purchase.

The current PartyTyme purchase API still sends payment fields as HTTPS GET query parameters, which may be retained in server, proxy, or application logs. This was **not** silently changed because the live third-party API contract has not been verified to support tokenization or a POST-body replacement. Recommended action: retire or redesign the purchase flow around a tokenized payment API before treating it as payment-security compliant.

### Remaining dependency warnings

The Release build still reports warnings from vendored legacy TagLib code and the pinned spdlog/fmt dependency. These are dependency-level warnings, not new first-party compile errors. Updating vendored dependencies should be handled as a separate compatibility-tested change.

### Catalog UI-thread work

The live song catalog payload is currently roughly 19.6 MB and is parsed/materialized on the GUI thread. The release improves network failure behavior but does not yet move catalog parsing to a worker thread.

### Test infrastructure

The repository still does not provide a comprehensive automated suite, but 3.0 adds CTest regression targets for malformed CD+G offsets/scroll paths and legacy payment-setting removal. Focused SQL, GStreamer-ownership, and cursor-boundary probes were also used for the defects above. The native Windows Release executable remained alive through the startup smoke interval and was deliberately terminated afterward; that deliberate termination is not reported as a crash.

## Compatibility

- No database schema changes.
- No command-line interface changes.
- Windows, Linux, and macOS native build targets are provided for 3.0.0.
- The GStreamer SDK path is now configurable instead of being tied to one machine.
