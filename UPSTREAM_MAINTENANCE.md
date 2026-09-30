# Upstream maintenance changes

Based on the community fork at `40c49be5`, reviewed on September 29, 2026.

## Included

- Adapted selected fixes from [upstream PR 306](https://github.com/OpenKJ/OpenKJ/pull/306): bound SQL for playlist imports, HTTPS, search-query encoding, strict update-version parsing, HTML escaping, verified CPM downloads, spdlog 1.17.0, compiler hardening, action pinning and explicit CodeQL compilation.
- Updated active workflows to a pinned Checkout 6.1.0 revision, covering [PR 300](https://github.com/OpenKJ/OpenKJ/pull/300).
- Adapted [PR 303](https://github.com/OpenKJ/OpenKJ/pull/303)'s application metadata improvements, with issue/source links pointing to this fork.
- Added regression coverage for quoted and Unicode playlist metadata, duplicate paths, transaction rollback, malformed update responses, and search query delimiters and percent escapes.
- Fixed two Windows compatibility issues encountered during verification: the forced QString formatter header now permits C translation units, and the column-layout test uses the same INI backend as the application.

Qt 6, the current release workflows, and existing application fixes are retained. Dependency PRs already covered by newer versions were omitted. The Nix packaging and broad chat/streaming proposals were not included. Container publishing remains disabled; `.dockerignore` now excludes Git history, local builds and environment files.

## Verification

- Full Windows x64 Release build: MSVC 19.44, Qt 6.7.3, installed MSVC GStreamer SDK, and the pinned spdlog revision fetched through CPM.
- All six CTest regression executables pass.
- The Windows binary contains Control Flow Guard metadata and a security cookie, and enables ASLR and NX.
- CPM smoke checks pass for a clean verified download, recovery from a corrupted cache, and rejection of an incorrect expected checksum.
- All five changed workflows pass actionlint 1.7.12. All workflow action references use full commit hashes. YAML and application XML parse successfully.
- HTTPS requests to the upstream version-file and song-search endpoints returned HTTP 200.

The Windows desktop automation helper was unavailable, so interactive startup, playback and dialog verification remain untested. Linux/macOS builds and hosted CodeQL execution also remain unverified. No release or deployment is included.
