# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MCCI's Arduino port of the IBM LMIC (LoRaWAN-MAC-in-C) framework, v6.0.0. Supports LoRaWAN 1.0.2/1.0.3 Class A and Class C devices on SX1272/SX1276/SX1261/SX1262 radios across EU868, US915, AU915, AS923, KR920, IN866 regions.

## Building and CI

This is an Arduino library -- there is no standalone build. Compilation happens through Arduino IDE, arduino-cli, or PlatformIO.

**CI (GitHub Actions):** `.github/workflows/ci-arduinocli.yml` builds with arduino-cli, one job per board for samd and stm32, one per region for esp32, and one for avr; the build lists live in `mcci-catena/mcci-catena-ci` (`arduino-lmic-regress-wrap.sh`), and the workflow narrows them per job with `MCCI_CI_BOARDS` and `MCCI_CI_REGIONS`. A `result` job ("Arduino CI result") passes only if every matrix job passed; it is the one check the branch rulesets require. A full run takes about 7 minutes. PlatformIO builds via `ci/platformio.sh`.

A `pull_request` run uses the workflow file from the PR's head branch. If a PR was branched before a workflow change, update it first so its checks match the ruleset: `gh api -X PUT repos/mcci-catena/arduino-lmic/pulls/NNNN/update-branch`. A first-time contributor's fork PR needs its workflow run approved (`gh run list --status action_required`, then `gh api -X POST .../actions/runs/ID/approve`).

**To compile-test locally** across several boards, use `scratch/compile-check.sh` (not checked in; see its `-h`).

**To compile an example locally with arduino-cli:**
```bash
arduino-cli compile --fqbn <board-fqbn> examples/ttn-otaa/ttn-otaa.ino
```

**Regression test compilation** uses `-DCOMPILE_REGRESSION_TEST` to bypass the requirement for user-supplied keys/config.

There are no unit tests or linting -- CI validates that examples compile across the target matrix.

## Configuration System (three layers, highest priority first)

1. **Compiler flags** (`-DCFG_us915`, `-DCFG_sx1276_radio`, etc.) -- override everything
2. **`project_config/lmic_project_config.h`** -- user-editable defaults for region and radio selection
3. **`src/lmic/config.h`** -- fallback defaults and compile-time validation

The config system enforces exactly one region and exactly one radio driver via preprocessor checks in `src/lmic/lmic_config_preconditions.h`. Violations produce `#error`.

Key suppression/redirection macros: `ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS`, `ARDUINO_LMIC_PROJECT_CONFIG_H`.

## Architecture

**Core LMIC** (`src/lmic/`): Pure C. The main state machine and LoRaWAN protocol implementation live in `lmic.c`. Event-driven via `osjob_t` callbacks scheduled with `os_setCallback()`/`os_setTimedCallback()`, single-threaded event loop driven by `LMIC_run()`.

**Radio drivers** (`src/lmic/radio_sx127x.c`, `radio_sx126x.c`): Selected at compile time by `CFG_sx1272_radio`/`CFG_sx1276_radio` or `CFG_sx1261_radio`/`CFG_sx1262_radio`. Common interface via `os_radio()` with command codes (`RADIO_RST`, `RADIO_TX`, `RADIO_RX`, etc.). Parameters and results passed through the global `LMIC` structure.

**Region bandplans** (`src/lmic/lmic_*region*.c`, `lorabase_*region*.h`, `lmic_bandplan_*region*.h`): Compile-time region selection -- one region per build. EU-like (EU868, AS923, IN866, KR920) and US-like (US915, AU915) share base implementations. See `doc/HOWTO-ADD-REGION.md` for the pattern.

**HAL** (`src/hal/`): C++ layer abstracting Arduino hardware. `hal.cpp` is the main implementation. Board-specific pin maps in `getpinmap_*.cpp` files. Runtime configuration via `HalPinmap_t` (pin assignments, SPI freq, RSSI cal) and virtual `HalConfiguration_t` class (TX power policy, TCXO, RF switch control).

**AES** (`src/aes/`): Two implementations selectable at compile time -- `USE_ORIGINAL_AES` (fast, large tables) or `USE_IDEETRON_AES` (compact, default for Arduino). AES interface abstracted via `lmic_aes_api.h` and `lmic_aes_interface.h`.

**Secure Element** (`src/se/`): Pluggable secure element abstraction for key storage and crypto operations. Default software implementation in `src/se/drivers/default/`. Interface defined in `src/se/i/lmic_secure_element_interface.h` and `lmic_secure_element_api.h`.

**Class C** (`LMIC_ENABLE_CLASS_C`): Continuous receive mode support. Enabled at compile time and configured at runtime via `LMIC_configureClassC()`. Class C state managed in `LMIC.classC` sub-structure. See `doc/CLASS-C.md` for usage. Breaking change from v5.x: `LMIC.rxtime` renamed to `LMIC.nextRxTime`.

**Public API entry point:** `src/arduino_lmic.h` (C). `src/lmic.h` is a deprecated C++ wrapper kept for backward compatibility.

**Doxygen:** `Doxyfile` at repo root for API documentation generation.

## Workflow

Two long-lived branches:

- **`main`** is the V6 line (6.1.x). Non-breaking changes only. Releases are tagged here. The Doxygen pages and the README badges follow it.
- **`v7-devel`** is the V7 line (7.0.0-preN). Breaking changes go here. It is always a superset of `main`: after anything merges to `main`, forward-merge `main` into `v7-devel`. At the V7 release, `v7-devel` merges into `main`, which is a fast-forward if the forward merges kept up.

Both branches have rulesets: every change goes through a pull request, the "Arduino CI result" check must pass, and there is no force-push, no deletion, and no bypass list, so this applies to maintainers too. The name `master` is retired and cannot be recreated.

**Non-breaking change** (V6.1 and V7 both get it): branch from `main`, PR to `main`, then forward-merge.

```
git checkout -b issueNNNN main
# ... make changes, commit ...
git push -u origin issueNNNN
gh pr create --base main --title "Short description (fixes #NNNN)" --body "..."
# wait for CI
gh pr merge PRNUM --merge --delete-branch
```

**V7-only change**: branch from `v7-devel`, PR to `v7-devel`.

```
git checkout -b issueNNNN-v7 v7-devel
# ... make changes, commit ...
git push -u origin issueNNNN-v7
gh pr create --base v7-devel --title "Short description (fixes #NNNN)" --body "..."
```

**Forward merge** of `main` into `v7-devel`, by PR like everything else:

```
git checkout -b fwd-main-v7devel v7-devel
git merge --no-ff origin/main
# expected conflicts: src/lmic/lmic_version.h (keep v7-devel's 7.0.0-preN)
#                     CHANGELOG.md (keep both sections, V7 above V6)
git checkout --ours src/lmic/lmic_version.h && git add src/lmic/lmic_version.h
git commit
# then advance v7-devel's pre counter in src/lmic/lmic_version.h, as its own commit
git commit -m "Advance version to 7.0.0-preN"
git push -u origin fwd-main-v7devel
gh pr create --base v7-devel --title "Merge main into v7-devel" --body "..."
```

Never merge `v7-devel` into `main` before the V7 release. Do not create a branch named `master`.

When merging by script, never pipe `gh pr merge` (that hides its exit status), and let `--delete-branch` do the deletion; do not delete the branch by hand.

## Code Conventions

- Tab size: 8, tabs not spaces (see `.vscode/settings.json`)
- Core is C with `extern "C"` wrappers for C++ interop; HAL layer uses C++ (`Arduino_LMIC` namespace)
- Custom fixed-width types: `u1_t`, `u2_t`, `u4_t`, `s1_t`, `s2_t`, `s4_t`, `ostime_t` (from `oslmic_types.h`)
- Pointer typedefs: `xref2name_t` pattern
- Function prefixes by module: `LMIC_*`, `os_*`, `radio_*`, `aes_*`
- Compile-time config uses `CFG_*` prefix; feature toggles use `LMIC_ENABLE_*` or `DISABLE_*`
- MIT license; maintain original IBM copyright notices; MCCI contributions attributed with year

## Key Documentation

- `doc/README.md` -- documentation index with links to all docs
- `doc/configuration.md` -- full configuration reference (all compile-time settings)
- `doc/CLASS-C.md` -- Class C usage guide and API reference
- `doc/timing.md` -- protocol timing, clock error, interrupt handling
- `doc/encoding-utilities.md` -- sflt16/uflt16/sflt12/uflt12 formats with JS decoders
- `doc/RadioDriver.md` -- radio driver interface specification
- `doc/HOWTO-ADD-REGION.md` -- step-by-step region addition guide
- `doc/HOWTO-Manually-Configure.md` -- hardware wiring and pin mapping
- `doc/LMIC-v5.0.0.pdf` -- API reference PDF (v6 update pending)
- `CHANGELOG.md` -- release history
- `README.md` -- landing page with links to detailed docs
- **GitHub Pages** (Doxygen): https://mcci-catena.github.io/arduino-lmic/

## Versioning Strategy

Version is encoded in `src/lmic/lmic_version.h` as `ARDUINO_LMIC_VERSION_CALC(major, minor, patch, pre)`. The `pre` field is the pre-release counter. `library.properties` is only updated at actual release time.

During development, version bumps follow these rules:

- **Patch fixes** on main: `X.Y.(Z+1)-preN` (e.g., 6.0.1-pre1)
- **Feature additions**: `X.(Y+1).0-preN` (e.g., 6.1.0-pre1)
- **Breaking changes**: `(X+1).0.0-preN` (e.g., 7.0.0-pre1)

Each line keeps its own number and its own `pre` counter: `main` is 6.1.0-preN, `v7-devel` is 7.0.0-preN. The commit that sets 7.0.0-pre1 is the first commit on `v7-devel` after the branch point. Every PR to `v7-devel`, including a forward merge, advances its `pre` counter, the same as on `main`. In a forward merge, resolve the `lmic_version.h` conflict by keeping `v7-devel`'s line and then advancing it.

The pre-release counter (`pre` field) increments with each version bump commit. On a feature branch, if you fix a bug in passing, bump `preN` -- don't change the patch/minor/major level mid-branch. The patch/minor/major level is set once when the branch is created and reflects the nature of the *most significant* change on the branch.

At release time, `pre` resets to 0 and `library.properties` is updated to match.

## Release Process

Reference: issue #978 documents the v5.0.0 release checklist.

To prepare a release (using `gh` CLI where possible):

1. **Choose version number** (semver: breaking = major, features = minor, fixes = patch)
2. **Update version in code:**
   - `src/lmic/lmic_version.h`: `ARDUINO_LMIC_VERSION_CALC(major, minor, patch, 0)`
   - `library.properties`: `version=X.Y.Z`
   - `Doxyfile`: `PROJECT_NUMBER = X.Y.Z`
3. **Update README.md badges:**
   - commits-since badge: change `compare/vOLD...main` to `compare/vNEW...main`
4. **Update CHANGELOG.md** with new version entry at top
5. **Update copyright dates** if year has changed (search for prior year)
6. **Update API documentation** (if applicable):
   - Update Word doc, rename with new version, regenerate PDF and redline
   - Update filename references in `doc/README.md` and `README.md`
   - Or defer to next release
7. **Create PR against `main`, get CI green, merge**
8. **Tag and create release** (annotated tag, always):
   ```bash
   git tag -a v$VERSION -m "v$VERSION"
   git push origin v$VERSION
   gh release create v$VERSION --title "v$VERSION: short description" --generate-notes
   ```
9. **Verify:** Arduino Library Manager picks up new version (may take hours)
10. **Forward-merge** `main` into `v7-devel` so the release commit is on both lines.

For the V7 release: forward-merge `main` one last time, then open a PR from `v7-devel` to `main` (a fast-forward if the forward merges kept up), and follow the steps above on `main`. If V6 needs a patch after that, cut `v6.x-maint` from the last v6 tag at that time.
