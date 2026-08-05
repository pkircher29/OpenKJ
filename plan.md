# OpenKJ Full Code Audit and Optimization Plan

Date: 2026-08-05
Baseline: `393f7f0661e80f364ff52ed332ef3bdc192380c7`
Working branch: `audit/full-optimization-2026-08-05`

## Objective

Produce an evidence-backed audit of OpenKJ's first-party code, fix reproducible correctness defects, and make measurable performance improvements without changing intended karaoke-show behavior. Keep the upstream branch untouched and separate vendored-code findings from OpenKJ-owned code.

## Scope

Primary scope:
- `src/` first-party application, models, CDG, GStreamer integration, database and file-scanning code
- Build definitions and CI workflows
- Windows x64 build and smoke testing on Paul's machine

Secondary scope:
- Cross-platform review of Linux/macOS conditionals
- Dependency age and integration risks

Excluded from direct refactoring unless required to fix an integration defect:
- Vendored TagLib and miniz internals
- Third-party spdlog internals
- Product redesign or feature expansion

## Ground Rules

1. Establish a clean build baseline before behavioral edits.
2. Reproduce a defect or identify a concrete violated invariant before fixing it.
3. Add regression coverage or a deterministic reproducer when practical.
4. Require benchmark evidence for performance claims.
5. Make small, reviewable commits; do not commit build outputs or user data.
6. Preserve `master`; all work stays on the audit branch.
7. Do not silently weaken error handling, logging, validation, or thread safety for speed.

## Audit Checklist

### 1. Baseline and build
- [x] Clone canonical upstream.
- [x] Record baseline commit and create protected working branch.
- [ ] Initialize recursive submodules.
- [ ] Reproduce the documented Windows Release build.
- [ ] Capture compiler warnings and build configuration.
- [ ] Record executable size and startup/smoke-test behavior.

### 2. Automated analysis
- [ ] Inventory first-party and vendored source separately.
- [ ] Run compiler warnings at a useful strictness without treating vendored warnings as product defects.
- [ ] Run available static analyzers (`clang-tidy`, Cppcheck, CodeQL-equivalent local checks where practical).
- [ ] Scan for unsafe ownership, lifetime, bounds, integer, format, SQL, path, archive, and concurrency patterns.
- [ ] Check Qt signal/slot ownership and thread-affinity hazards.
- [ ] Check error paths, ignored return values, and exception boundaries.
- [ ] Audit build/CI dependency pinning and cross-platform assumptions.

### 3. Domain-focused manual review
- [ ] Database queries, migrations, transactions, and model invalidation.
- [ ] Karaoke library scanning, filename parsing, ZIP/archive handling, and malformed inputs.
- [ ] GStreamer pipeline lifecycle, callbacks, and audio/video resource cleanup.
- [ ] CDG parsing/rendering bounds and timing behavior.
- [ ] Rotation, queue, request, and singer-state invariants.
- [ ] Network/API parsing, TLS assumptions, retries, and cancellation.
- [ ] Settings, secrets-adjacent values, logs, and local file permissions.

### 4. Optimization
- [ ] Profile or instrument likely hot paths before changing them.
- [ ] Inspect database query count, indexes, transaction batching, and repeated model refreshes.
- [ ] Inspect library scan allocations, regex compilation, archive reopening, and metadata reads.
- [ ] Inspect render/audio loops for avoidable copies, allocations, locks, and UI-thread work.
- [ ] Benchmark each accepted optimization against the baseline.
- [ ] Reject cosmetic micro-optimizations without meaningful evidence.

### 5. Verification
- [ ] Add focused regression tests or deterministic harnesses for accepted fixes.
- [ ] Rebuild Debug and Release configurations.
- [ ] Run automated tests and smoke tests.
- [ ] Re-run analyzers and compare warning counts.
- [ ] Review final diff for accidental behavior/API changes.
- [ ] Produce a ranked report: fixed, confirmed-open, suspected, and out-of-scope findings.
- [ ] Include exact commands, outputs, limitations, and benchmark results.

## Execution Order

1. Baseline inventory and reproducible Windows build.
2. Parallel audits by subsystem.
3. Consolidate findings by severity and confidence.
4. Fix correctness defects first, each with verification.
5. Profile and optimize only measured bottlenecks.
6. Full clean rebuild, smoke test, analyzer rerun, and final report.

## Definition of Done

The task is done only when the branch builds from a clean tree, accepted fixes have evidence, performance claims have before/after measurements, the final diff has been reviewed, and remaining risks and environmental limitations are plainly documented.
