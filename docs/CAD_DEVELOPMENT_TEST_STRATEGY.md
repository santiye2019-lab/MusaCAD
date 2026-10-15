# MusaCAD development test cadence — incremental CAD work

This document defines **how to accelerate development without removing required quality gates**.

## Three independent levels

1. **Fast feedback on each relevant CAD change:** `.github/workflows/cad-fast-regression.yml` executes targeted Python and native C regressions without cloning/building the full LibreDWG toolchain or creating an APK. A passing fast run is **only** proof for the covered assertions.
2. **Complete GitHub quality gates:** `android.yml` and `cad-r2000-fidelity.yml` remain authoritative for their respective coverage, including actual Android build checks and modern/legacy DWG fixtures. They can execute while other implementation work continues. A cancelled, queued, skipped or failed run **never means PASS**.
3. **Final device acceptance:** after all source modules, required full CI and independent acceptance checks are complete, create one consolidated **test** APK; complete the defined 18 physical-device checks before production acceptance. Only then may the final production branch/release be considered. The source `main` and existing APK remain unchanged during development.

## Day-to-day development

- Work in the feature branch and group related changes into coherent batches; avoid unnecessary commits while expensive CI is running because `cancel-in-progress` may supersede an older run.
- Run the nearest deterministic unit tests first; fix failing tests or implementation immediately.
- Do not wait idly for long full-CI work. Develop the **next independent** module while CI runs; if a run fails, return to that failure before declaring its module complete.
- Record successful tests with their exact **commit SHA**. Do not transfer PASS status from one SHA to newer untested changes.
- No required checks may be disabled, bypassed or classified as successful merely to increase the progress metric.

## Completion accounting

Development progress is **planning points for implemented, tested milestones**, not AutoCAD compatibility and not smartphone acceptance. Record points in `docs/CAD_2026_PROGRESS.json` only after the milestone has real code plus passing required tests. A workflow refactor does not, by itself, increase CAD feature coverage. Report the old and new percentage and open required work after each verified milestone.

The deliverable target is **zero failing modules, zero failing required checks and zero silently skipped mandatory tests**, with independent source-DWG parity evidence where applicable.
