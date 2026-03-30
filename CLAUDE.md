# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MCCI's Arduino port of the IBM LMIC (LoRaWAN-MAC-in-C) framework, v6.0.0. Supports LoRaWAN 1.0.2/1.0.3 Class A and Class C devices on SX1272/SX1276/SX1261/SX1262 radios across EU868, US915, AU915, AS923, KR920, IN866 regions.

## Building and CI

This is an Arduino library -- there is no standalone build. Compilation happens through Arduino IDE, arduino-cli, or PlatformIO.

**CI (GitHub Actions):** `.github/workflows/ci-arduinocli.yml` builds across samd, stm32, esp32, avr architectures using arduino-cli. PlatformIO builds via `ci/platformio.sh`.

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

All changes go through pull requests, never direct pushes to master. This applies even for small fixes -- it sets a good example for contributors.

```
git checkout -b issueNNNN master
# ... make changes, commit ...
git push -u origin issueNNNN
gh pr create --title "Short description (fixes #NNNN)" --body "..."
# wait for CI
gh pr merge PRNUM --merge --delete-branch
```

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

Version is encoded in `src/lmic/lmic.h` as `ARDUINO_LMIC_VERSION_CALC(major, minor, patch, local)`. The `local` field serves as a pre-release counter. `library.properties` is only updated at actual release time.

During development, version bumps follow these rules:

- **Patch fixes** on master: `X.Y.(Z+1)-preN` (e.g., 6.0.1-pre1)
- **Feature additions**: `X.(Y+1).0-preN` (e.g., 6.1.0-pre1)
- **Breaking changes**: `(X+1).0.0-preN` (e.g., 7.0.0-pre1)

The pre-release counter (`local` field) increments with each version bump commit. On a feature branch, if you fix a bug in passing, bump `preN` -- don't change the patch/minor/major level mid-branch. The patch/minor/major level is set once when the branch is created and reflects the nature of the *most significant* change on the branch.

At release time, `local` resets to 0 and `library.properties` is updated to match.

## Release Process

Reference: issue #978 documents the v5.0.0 release checklist.

To prepare a release (using `gh` CLI where possible):

1. **Choose version number** (semver: breaking = major, features = minor, fixes = patch)
2. **Update version in code:**
   - `src/lmic/lmic.h`: `ARDUINO_LMIC_VERSION_CALC(major, minor, patch, 0)`
   - `library.properties`: `version=X.Y.Z`
   - `Doxyfile`: `PROJECT_NUMBER = X.Y.Z`
3. **Update README.md badges:**
   - commits-since badge: change `compare/vOLD...master` to `compare/vNEW...master`
4. **Update CHANGELOG.md** with new version entry at top
5. **Update copyright dates** if year has changed (search for prior year)
6. **Update API documentation** (if applicable):
   - Update Word doc, rename with new version, regenerate PDF and redline
   - Update filename references in `doc/README.md` and `README.md`
   - Or defer to next release
7. **Create PR, get CI green, merge**
8. **Tag and create release:**
   ```bash
   git tag v$VERSION
   git push origin v$VERSION
   gh release create v$VERSION --title "v$VERSION: short description" --generate-notes
   ```
9. **Verify:** Arduino Library Manager picks up new version (may take hours)
