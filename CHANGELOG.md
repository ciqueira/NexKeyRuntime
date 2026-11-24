# Changelog

All notable changes to the NexKeyRuntime public repository will be
documented in this file.

## [1.4.6] - 2026-11-24

### Added

- **Opt-In MIDI Port Selection & Coexistence (`nexcontrol_hub`):** Added `nexcontrol_hub_set_port_enabled()` and extended `NexControlDeviceInfo` (preserving ABI compatibility) with `is_enabled` and `connection_state` (`NexControlPortState`: `Closed`, `Open`, `Busy`) so desktop hubs only open explicitly authorized MIDI input ports, preventing exclusive MIDI port conflicts on Windows and duplicate input handling on macOS alongside other MIDI apps.
- **Generic MIDI Auto-Detect & Encoder Modes (`nexcontrol_hub`):** Added `NEXCONTROL_GENERIC_MODE_*` constants (`0..4`) and updated `nexcontrol_hub_set_generic_encoder_mode()` with range validation (returning `-1` for out-of-range values). Generic MIDI controllers now default to `Auto-Detect` (`4`), apply mode changes live across active generic sessions, and maintain isolated per-controller state so multiple generic controllers operate concurrently without cross-talk.

### Changed

- **Generic MIDI Parameter Mapping (NexControl):** Adjusted built-in generic CC mappings (`CC 1..31` and `CC 33..119`) to start at parameter index `0` so the first plugin parameter is directly controllable from generic MIDI hardware.

### Fixed

- **MIDI Port Lifecycle & Teardown Reliability (`nexcontrol_hub`):** Eliminated duplicate port discovery on startup, improved disambiguation for ports with identical or Unicode names, prevented single-port teardown from resetting other active controllers, suppressed false `Unmatched` diagnostics during initial encoder calibration, and hardened profile and diagnostic telemetry synchronization.
- **macOS OpenFX Unload & Teardown Stability (NexControl):** Fixed potential crashes when a host unloads a NexControl-enabled plugin bundle while hardware focus or parameter callbacks are still pending, and improved shutdown safety across threads.
- **Parameter Lifecycle & Instance Isolation (NexControl):** Hardened parameter slot lifetimes and instance recycling during rapid plugin creation, destruction, and teardown to prevent race conditions.
- **macOS Viewer Refresh Shutdown Cleanup:** Improved cleanup of background viewer-refresh operations on macOS for faster plugin unload without leaving orphaned host helper processes.

## [1.4.5] - 2026-11-23

### Changed

- **Dedicated DCTL Packager Library (`nexkeypackager`):** Extracted DCTL preprocessing and asset packaging into a separate static library (`libnexkeypackager.a` on macOS, `nexkeypackager.lib` on Windows; CMake target `NexKeyRuntime::Packager`). Plugins and host apps linking `nexkeyruntime` stay minimal, while `nexkeyctl` links both `nexkeyruntime` and `nexkeypackager`.

### Security

- **Runtime, Storage & Asset Packaging Hardening:** Strengthened local receipt and credential handling, hardened content-key access and in-memory cleanup, and tightened asset packaging and Universal DCTL Runner environment isolation. Integrators shipping `NexKeyRuntime` or `DCTL-to-OFX` builds are encouraged to update to `1.4.5`.

## [1.4.4] - 2026-11-22

### Added

- **NexControl Parameter Grouping & Batching (`nexcontrol.h` / `MCNexControl.h`):** Added `nexcontrol_add_param_ex`, `nexcontrol_begin_batch`, `nexcontrol_end_batch`, `nexcontrol_notify_user_edit`, and `nexcontrol_set_focus_query_override`, along with parameter display flags (`NEXCONTROL_PARAM_FLAG_*`) and C++ helpers (`BatchScope`, `GroupScope`).
- **Virtual Bank Navigation & Diagnostics (`nexcontrol_hub`):** Added virtual bank pagination across grouped plugin parameters (`nexcontrol_hub_get_active_bank_map`, `nexcontrol_hub_step_virtual_bank`) and extended `NexControlTelemetrySnapshot` with bank and MIDI diagnostic fields.
- **MIDI Profile Banking Schema Extensions:** Extended `schemas/midi-device-profile.schema.json` with optional `knobs_per_page` and `virtual_bank_controls` settings for hardware bank navigation.

### Changed

- **Refined Encoder Scaling & Precision:** Improved encoder scaling across OpenFX parameters and the Universal DCTL Runner so slow turns step by 1 unit of the last displayed digit and fast turns accelerate smoothly across the display range; updated `step_sensitivity` defaults in `schemas/midi-device-profile.schema.json` to `0.0005` (`minimum: 0.00005`).
- **Windows Focus Responsiveness & Profile Reload Stability:** Improved active-node focus responsiveness on Windows during continuous MIDI control and improved live hardware profile reload stability in the desktop hub.

## [1.4.3] - 2026-11-20

### Fixed

- **Windows OpenFX Plugin Load & Shutdown (NexControl):** Fixed a startup hang when loading NexControl-enabled plugins in DaVinci Resolve on Windows and improved dispatch teardown reliability on plugin unload.
- **OpenFX Undo Grouping & Windows Viewer Refresh:** Grouped hardware-driven parameter changes into proper host undo transactions and improved Windows viewer-refresh responsiveness and shutdown cleanup.
- **Spurious Parameter Jump Prevention:** Prevented queued or stale deltas, concurrent reset/toggle actions, and MIDI controller reconnects from causing sudden parameter jumps.

## [1.4.2] - 2026-11-19

### Added

- **Unified Root CMake Build & Documentation:** Added a root `CMakeLists.txt` and shared `cmake/NexKeyRuntimeSetup.cmake` helper to build `nexkeyctl`, all minimal examples (`build/bin/`), and optional OpenFX plugins (`-DNEXKEY_BUILD_OFX=ON` into `build/plugins/`) from the repository root, alongside a manual quickstart guide.
- **NexControl Libraries in Release Packages:** Precompiled `nexcontrol` and `nexcontrol_hub` binaries and headers are now included in the macOS and Windows release archives.

### Changed

- **Streamlined `nexkeyctl` & DCTL Sandbox Paths:** `nexkeyctl` now defaults to `mcnexus-sandbox`, auto-detects `sandbox/mcnexus-sandbox.nexkeydat` and platform sandbox runners when run inside the repository, and standardizes packaged DCTL output to `dist/`.
- **Windows OpenFX Bundle Packaging:** Updated the OpenFX examples and sandbox plugin CMake targets to assemble `.ofx.bundle\Contents\Win64\` on Windows and match the static `/MT` Release configuration.
- **Local CLI Builds:** Removed prebuilt `nexkeyctl` binaries from release archives in favor of building locally from `tools/nexkeyctl`.

## [1.4.1] - 2026-11-17

### Added

- **Windows Active-Node Focus Tracking (NexControl):** Added on-demand automatic active-node focus tracking for DaVinci Resolve on Windows inside the desktop hub, matching macOS multi-instance isolation without requiring Accessibility permissions from plugins.
- **Expanded MIDI Hardware Profiles & Encoder Modes:** Built-in support for **Behringer X-TOUCH MINI** (supporting `sign_magnitude` and `absolute_step` encoder modes) alongside **Midi Fighter Twister** and **MCMIDI Atom MIDI Hub**, plus reference JSON profiles for **Arturia BeatStep** and **Korg nanoKONTROL Studio** (`schemas/midi-device-profile.schema.json` extended with `sign_magnitude` and `absolute_step`).
- **Windows Hardware Control Transport Parity:** Completed Windows MIDI hub IPC transport parity and opt-in build packaging support alongside macOS.

### Changed

- **Lightweight Plugin Client Builds:** Decoupled the OpenFX hardware-control client library from MIDI host enumeration dependencies so plugin builds only link what they need.
- **Universal DCTL Runner & Sandbox Diagnostics:** Updated precompiled universal OpenFX runner modules and enhanced sandbox debug logging with NexControl hub/focus status and accurate version display.

## [1.4.0] - 2026-11-08

### Added

- **NexControl Hardware Subsystem**:
  - Real-time hardware control integration for OpenFX plugins with external MIDI surfaces (Midi Fighter Twister and custom MIDI controllers).
  - Low-latency binary IPC transport and relative encoder support (Two's Complement and Binary Offset 64).
  - Multi-tier device profiles: built-in certified mappings, generic MIDI fallback, and customizable JSON device profiles.
  - Automatic in-process active node tracking in DaVinci Resolve without requiring OS Accessibility permissions.
  - Multi-plugin instance isolation ensuring independent hardware control across different tools on the same timeline.
  - Full support for continuous sliders (`Double`), discrete dropdown selectors (`Choice`), and boolean switches (`Boolean`).
  - Seamless timeline viewer refresh during playback pause.
- **Universal DCTL Runner NexControl Integration**:
  - Automatic discovery and dynamic binding of DCTL UI parameters to physical hardware controls.
  - Embedded `grpNexControl` parameter group with status indicator for the DaVinci Resolve Inspector.
- **Reference Example & Schemas**:
  - Added reference OpenFX example in `examples/06-nexcontrol-ofx/`.
  - Added JSON schema for MIDI controller profile validation in `schemas/midi-device-profile.schema.json`.

## [1.3.0] - 2026-11-04

### Added

- **DCTL-to-OFX Sandbox Testbed (`sandbox/dctl-to-ofx/`)**:
  - Evaluation environment allowing colorists and developers to package DCTL color algorithms and 3D LUTs into native OpenFX plugins for DaVinci Resolve Studio.
  - Zero C++ compiler requirements for creators: shaders are packaged and run via the precompiled universal OpenFX runners.
  - Minimal reference DCTL script (`Exposure.dctl`) demonstrating interactive UI sliders.
- **Precompiled Universal OpenFX Binary Runners**:
  - Neutral, immutable binary modules (`NexDCTLRunner.ofx`) bundled in `sandbox/dctl-to-ofx/runners/` for macOS (Universal arm64/x86_64 via Metal) and Windows (x64 via OpenCL).
  - Runtime GPU shader compilation and caching for smooth playback.
- **CLI Packaging & Sandbox Control (`nexkeyctl`)**:
  - `nexkeyctl package-dctl`: Automates preprocessing of standard DaVinci Resolve DCTL macros (`DEFINE_UI_PARAMS`, `DEFINE_LUT`, `#include`) into an OpenFX bundle.
  - `--sandbox` flag: Prepares bundles for immediate local testing locked to tenant `mcnexus-sandbox`.
  - `--debug` flag: Captures GPU shader compilation and lifecycle diagnostics to a local log file with zero impact on real-time playback.

### Fixed

- **Credential Lifecycle Management**: Resolved an issue where read-only license handles could unexpectedly rewrite previously cleared credentials during background poll cycles.
- **Dynamic License Synchronization**: Background polling and on-demand synchronization routines now dynamically reflect updated credentials across active license handles.

## [1.2.1] - 2026-11-03

### Fixed

- **Multi-Entitlement Credential Isolation**: Fixed an issue where multiple products or entitlements sharing the same tenant directory could overwrite each other's cached license keys. Credentials are now stored in isolated per-entitlement stores.
- **Symmetric Product Deactivation**: Deactivating an individual license handle now targets only that specific entitlement's store, preventing unintended revocation or clearing of sibling licenses.

## [1.2.0] - 2026-10-01

### Changed

- SDK activation and sync requests now include the entitlement configured on
  the license handle. The backend can issue a certificate for that specific
  variant when a license grants multiple download entitlements. The public C
  API and certificate format are unchanged; single-entitlement licenses remain
  compatible.

## [1.1.0] - 2026-09-30

### Added

- **`NexKeyRuntimeScope` enum and `nexkeyruntime_license_set_scope()`:** Callers can choose between user-specific storage (`NEXKEYRUNTIME_SCOPE_USER`, default) and machine-wide storage (`NEXKEYRUNTIME_SCOPE_SYSTEM`). Must be called before `nexkeyruntime_license_load_local()` or `nexkeyruntime_license_activate()`.

  Platform mapping:

  | Scope | Windows | macOS |
  |---|---|---|
  | `USER` | `%LOCALAPPDATA%\NexKeyRuntime\<tenant>` | `~/Library/Application Support/NexKeyRuntime/<tenant>` |
  | `SYSTEM` | `%PROGRAMDATA%\NexKeyRuntime\<tenant>` | `/Library/Application Support/NexKeyRuntime/<tenant>` |

  Replaces the hardcoded `%PROGRAMDATA%` path introduced in `1.0.1` so applications and plugins use per-user storage by default without requiring elevation. `USER` and `SYSTEM` handles for the same tenant and variant synchronize independently. Additive change; `1.0.x` code recompiles untouched.

## [1.0.1] - 2026-09-19

Windows only. No API changes.

### Fixed

- **MSIX & Win32 Receipt Sharing on Windows:** Fixed an issue where receipts written by MSIX-packaged apps were isolated from standard Win32 host applications (such as DaVinci Resolve). Windows receipts were moved to `%PROGRAMDATA%` so both packaged apps and plugins can read them consistently.

## [1.0.0] - 2026-09-04

No API changes from `0.5.4`. Establishes the `1.x` ABI stability commitment documented in `docs/ABI_POLICY.md` (append-only result codes, `struct_size`-versioned structs, opaque handles, additive `1.x` releases) and finalizes `BINARY_LICENSE.md` for third-party developers.

## [0.5.4]

No API changes.

### Fixed

- **Background Sync Under Continuous Rendering:** Fixed an issue where continuous calls to `nexkeyruntime_license_render_decision()` during playback, scrubbing, or preview could defer background license refreshes indefinitely. Background sync now runs periodically even under continuous render load so revocations, suspensions, and updates propagate without waiting for idle time, while keeping render calls responsive.

## [0.5.3]

Windows only. No API changes.

### Fixed

- **Windows-on-ARM Architecture Detection:** Fixed an issue where Windows-on-ARM machines reported `x86_64` instead of `arm64` during activation. The SDK now detects and reports the host's actual runtime architecture on Windows.

## [0.5.2]

No API changes.

### Fixed

- **Per-Entitlement Deactivation Cleanup:** `nexkeyruntime_license_deactivate()` and `nexkeyruntime_license_export_deactivation_proof()` now clear only the local receipt for the handle's own entitlement instead of removing sibling product receipts under the same tenant. Stored license keys are retained while sibling entitlements still depend on them.
- **Offline Deactivation Proof Scoping:** `nexkeyruntime_license_export_deactivation_proof()` now resolves the activation strictly from the handle's own entitlement receipt and returns `NEXKEYRUNTIME_E_NO_RECEIPT` when no matching receipt exists.

## [0.5.1]

Windows only. No API changes.

### Fixed

- **Windows Static CRT Linkage (`/MT`):** Updated the published `nexkeyruntime.lib` to link against the static C runtime (`/MT` Release, `/MTd` Debug), resolving `LNK2038` / `LNK2005` runtime-library mismatches in default MSVC builds and removing the external VC++ Redistributable dependency.

## [0.5.0]

Additive only; `0.4.0` code recompiles untouched and existing activations remain valid.

### Added

- **Offline (Air-Gapped) Activation & Deactivation:** Added two functions for environments without direct network access:
  - `nexkeyruntime_license_export_activation_request()` writes a JSON request containing the machine's derived binding, entitlement, and license key prefix. Pass the resulting certificate to `nexkeyruntime_license_publish_receipt()`.
  - `nexkeyruntime_license_export_deactivation_proof()` clears local receipts immediately and writes a deactivation proof to release the seat offline. Returns `NEXKEYRUNTIME_E_NO_RECEIPT` when no matching receipt exists.
  - Both functions enforce the same preconditions as `nexkeyruntime_license_load_local()` and follow the same seat limits as online activation.

## [0.4.0]

No API changes; `0.3.0` code recompiles untouched and existing activations remain valid.

### Added

- **SDK Version Reporting:** Activation and sync requests now automatically report the SDK version.
- **Application Name Metadata (`set_metadata("product", ...)`):** Activations now record the integrating application or plugin name when provided via `"product"`, allowing multiple apps sharing an entitlement on the same machine to be distinguished cleanly.
- **Operating System Version Metadata:** Activation and sync requests now report the non-identifying OS version on macOS and Windows.

## [0.3.0]

**Migration required:** Existing `0.2.x` activations must be re-activated. No API changes (`0.2.x` code recompiles untouched), but `0.2.x` certificates report `DEVICE_MISMATCH` under `0.3.0`.

### Fixed

- **Cross-Build Machine Binding Consistency:** Standardized machine binding computation across all supported build configurations so different binaries on the same workstation always produce the same binding.

### Added

- **`fingerprintVersion` Tracking:** Activation and sync requests now include `fingerprintVersion` (`2` for the standardized binding; `1` for legacy pre-`0.3.0` clients).

### Migrating from 0.2.x

1. Upgrade all host applications and plugins on the machine to `0.3.0` together.
2. Release legacy activations (`fingerprintVersion = 1`) for the affected licenses.
3. Re-activate.

## [0.2.3]

No API changes.

### Fixed

- **Startup Clock-Skew Tolerance:** Added a bounded tolerance on `notBefore` during certificate verification so machines slightly behind server time (e.g., studio or render-farm nodes without NTP) do not reject freshly activated certificates. Expiration (`expiresAt` / `offlineValidUntil`) and rollback checks remain strict.

### Note for Render Farms and Headless Nodes

Headless nodes perform local verification only and do not run background sync. Size the offline grace period to exceed the longest interval between renewals or re-activations.

## [0.2.2]

macOS only. No API changes.

### Fixed

- **macOS Network Timeout Hang:** Fixed an issue where a stalled HTTP request on macOS could block past its configured timeout and hang plugin unload during `nexkeyruntime_license_destroy()`. Network calls now return reliably once the timeout elapses.

## [0.2.1]

No API changes.

### Fixed

- **Stale Key Sync Cleanup:** Fixed an issue where switching license keys on a tenant could cause background sync to remove valid receipts for the newly activated license. Sync requests now refresh from current state each cycle and only affect the matching receipt.
- **`last_synced_at` Population:** Fixed `NexKeyRuntimeLicenseSnapshot.last_synced_at` remaining `0` after successful server syncs.
- **macOS Non-Standard `$HOME` Storage:** Fixed storage initialization failing on macOS when intermediate parent directories under `$HOME` did not yet exist.

## [0.2.0]

### Added

- **`nexkeyruntime_license_set_callback()`:** Subscribe to background sync results on the poller thread so hosts receive suspension or revocation updates without polling. Callbacks must be non-blocking and must not re-enter the handle.

### Changed

- **Online Licensing (Profile B):** Implemented `nexkeyruntime_license_set_license_key`, `_set_metadata`, `_activate`, `_deactivate`, `_publish_receipt`, `_request_sync`, and `_set_headless` for online activation, deactivation, and background sync. Offline (Profile A) integrations remain unaffected.
- **Clean Thread Shutdown on Destroy:** `nexkeyruntime_license_destroy()` now stops and joins the background sync thread before returning so no SDK thread outlives plugin unload.

### Added (Initial Public Scaffold)

- Initial public repository release including Apache-2.0 `LICENSE`/`NOTICE`, `README.md`, `CONTRIBUTING.md`, `SECURITY.md`, draft `BINARY_LICENSE.md`, public C header (`include/nexkeyruntime/nexkeyruntime.h`), integration and ABI policy docs, JSON Schemas, and a minimal C example.
