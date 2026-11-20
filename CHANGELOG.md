# Changelog

All notable changes to the NexKeyRuntime public repository will be
documented in this file.

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
  - Shaders are compiled and cached directly on the host GPU at runtime with zero playback stalls.
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

- **`NexKeyRuntimeScope` enum and `nexkeyruntime_license_set_scope()`**: callers can now choose whether the license state is stored and read from a **user-specific** location (`NEXKEYRUNTIME_SCOPE_USER`, the new default) or a **machine-wide** location (`NEXKEYRUNTIME_SCOPE_SYSTEM`). Must be called before `nexkeyruntime_license_load_local()` or `nexkeyruntime_license_activate()`. If not called, behavior is identical to previous releases (`NEXKEYRUNTIME_SCOPE_USER`).

  Platform mapping:

  | Scope | Windows | macOS |
  |---|---|---|
  | `USER` | `%LOCALAPPDATA%\NexKeyRuntime\<tenant>` | `~/Library/Application Support/NexKeyRuntime/<tenant>` |
  | `SYSTEM` | `%PROGRAMDATA%\NexKeyRuntime\<tenant>` | `/Library/Application Support/NexKeyRuntime/<tenant>` |

  This replaces the hardcoded `%PROGRAMDATA%` path introduced in 1.0.1 for MSIX compatibility. Applications (such as MCNexus) and plugins should now use the default `USER` scope (`NEXKEYRUNTIME_SCOPE_USER`), which requires no elevation, is not subject to File System Virtualization, and correctly segregates license data per-user.

  The background poller's shared-poller key now includes the scope, so a `USER` handle and a `SYSTEM` handle for the same tenant-and-variant pair no longer share a single poller.

  No existing API changed. 1.0.x code recompiles against this header untouched; the new `set_scope` call is purely optional and additive.

## [1.0.1] - 2026-09-19

Windows only. No API changed.

### Fixed

- **MSIX AppContainer virtualization blocked receipts from being read by non-packaged apps.** When the SDK was activated inside an MSIX packaged app (like MCNexus), Windows File System virtualization redirected writes to `%LOCALAPPDATA%` into a hidden `LocalCache` directory within the app's isolated container. OFX plugins running inside standard Win32 host applications (like DaVinci Resolve) read the real `%LOCALAPPDATA%`, failed to find the receipt, and denied the render. The SDK now stores receipts in `%PROGRAMDATA%` on Windows, which is not virtualized for full-trust MSIX apps and is reliably readable by all applications.

## [1.0.0] - 2026-09-04

No API change from 0.5.4: 0.5.4 code recompiles against this header
untouched. This release is a stability declaration, not a functional one:
the compatibility guarantees `docs/ABI_POLICY.md` already documented
(append-only result codes, structs that only grow via `struct_size`, opaque
handles, no allocator crossing the ABI boundary) are now a committed
promise. From here, a breaking change requires `2.0`; `1.x` releases are
additive only.

Also, `BINARY_LICENSE.md` (the license governing these compiled binaries
for third-party developers who statically link them into their own
product) is finalized, clearing them for use by developers outside Nexus.

## [0.5.4]

No API changed, so 0.5.3 code recompiles against this header untouched. The
background poller now behaves differently in one case, described below.

### Fixed

- **The background poller never synced inside a host that keeps
  rendering.** `nexkeyruntime_license_render_decision()` is the render
  guard's hot path, and it tells the poller that a render happened so the
  background refresh can stay out of the way while frames are being produced.
  The refresh answered by yielding for a short interval, with no ceiling. A
  host calling `render_decision()` more often than that interval therefore
  yielded for ever, and the refresh never ran. Hosts call the render guard on
  every frame of playback, every scrub and every preview, so this was the
  normal case, not an edge one.

  The effect was that **nothing from the server reached a plugin while it was
  being used**: not revocation, not suspension, not expiry, not
  deactivation. The license kept being honored on the strength of the
  offline window, which for a typical certificate is 30 days.

  Under identical test conditions with a live backend, an idle process synced
  and refreshed its receipt while a continuously rendering process stayed on
  its cached certificate.

  Yielding is now bounded: under continuous rendering the refresh still runs
  periodically so revocations and updates propagate without waiting for idle
  time. Renders still take priority during active frame processing, and
  integrators need to do nothing; the SDK schedules this on its own.

## [0.5.3]

Windows only. No API changed, so 0.5.2 code recompiles against this header
untouched.

### Fixed

- **Windows-on-ARM machines activated as `x86_64` instead of `arm64`.** The
  architecture reported at activation was fixed at compile time, so it
  reflected the environment the binary was built for rather than the
  machine it was actually running on; every Windows-on-ARM activation was
  indistinguishable from a genuine x64 one. The SDK now determines the
  host's real architecture at runtime on Windows and reports that.

## [0.5.2]

No API changed, so 0.5.1 code recompiles against this header untouched. One
return code now fires in a case where it previously did not: see the second
entry below.

### Fixed

- **Giving up one seat deleted every receipt on the machine.** Both
  `nexkeyruntime_license_deactivate()` and
  `nexkeyruntime_license_export_deactivation_proof()` cleared all local receipts
  for the tenant rather than scoping cleanup to the handle's entitlement. Since
  several products under one tenant share the machine while deactivation
  releases only one seat matched by tenant, license key, machine binding, and
  entitlement, deactivating product A removed product B's local receipt on a
  machine that remained licensed. Each operation now clears only the receipts
  for its own handle's entitlement, leaving sibling products intact.

  The stored license key follows the same rule and is removed only when no
  other receipt under that tenant still needs it, so sibling products keep
  renewing normally.

- **The offline deactivation proof could name an unrelated activation, and
  could be written when this handle held no seat at all.** On a multi-product
  tenant, `export_deactivation_proof()` could resolve an `activationId` from a
  sibling entitlement or return `NEXKEYRUNTIME_OK` when this handle had no seat.
  It now reads the `activationId` strictly from this handle's own entitlement
  receipt before clearing it, and returns `NEXKEYRUNTIME_E_NO_RECEIPT` when no
  matching receipt exists.

## [0.5.1]

Windows only. No API changed, so 0.5.0 code recompiles against this header
untouched.

### Fixed

- **The published `nexkeyruntime.lib` linked the dynamic CRT, which a plain
  `cl` build never asked for.** CMake's own default for MSVC targets
  (`CMP0091`) is `/MD`, and nothing in this SDK's build ever overrode it, so
  every Windows release since 0.1.0 shipped that way. A consumer compiled the
  ordinary way (no explicit runtime-library flag, which makes `cl` default
  to `/MT`) got `LNK2038: mismatch detected for 'RuntimeLibrary'` at link
  time against this library, plus `LNK2005` duplicate CRT symbols and an
  unresolved `__imp_remove`, all the same root cause. Found integrating 0.5.0
  into a real OFX plugin. `nexkeyruntime.lib` now links the static CRT
  (`/MT` release, `/MTd` debug) instead, matching what a default `cl` build
  already expects and keeping this library free of a VC++ Redistributable
  requirement on the machine that runs the final binary, which is the same
  self-contained posture the vendored Ed25519 backend already has.

## [0.5.0]

Additive only: nothing in the existing surface moved, was renumbered, or
changed behavior, so 0.4.0 code recompiles untouched and every existing
activation keeps working.

### Added

- **Offline (air-gapped) activation and deactivation.** Until now, every
  activation and deactivation reached the network directly, so a machine
  that cannot (the backend is unreachable, or the install is deliberately
  air-gapped) had no way in or out. Two new functions cover that round
  trip without this SDK ever touching the network itself:

  - `nexkeyruntime_license_export_activation_request()` writes a small JSON
    request file carrying this machine's derived binding (never raw hardware
    identifiers), the entitlement, and a short prefix of the configured license
    key. Whatever comes back in response is a certificate: hand it to
    `publish_receipt()`, not to this function.
  - `nexkeyruntime_license_export_deactivation_proof()` stops this machine's
    license and writes the deactivation proof, again without touching the
    network. Local receipts are cleared **before** the proof is written so a
    deactivated workstation stops rendering immediately; forward the proof file
    unmodified to release the seat. Returns `NEXKEYRUNTIME_E_NO_RECEIPT` when
    there is nothing to give up.

  Both functions share the same preconditions as `load_local()`
  (`set_product_data`/`set_product_file` and `set_tenant_id` must already
  have succeeded), and whatever answers the exported request goes through
  the same activation path as the online flow: the same seat limit
  applies. Offline activation is a different transport for the same
  decision, not a side door around it.

## [0.4.0]

No API changed and no stored state is affected, so 0.3.0 code recompiles
untouched and activations made against it keep working. What changes is what
the SDK reports about itself and the machine.

### Added

- **The SDK now reports its own version on every activation and sync.** It was
  the one participant that could not be wrong about it (no integrator has to
  remember a call, and no build can claim a version it is not), yet it was the
  one thing never sent. Until now the only way to tell which SDK produced an
  activation was to infer it from `fingerprintVersion`, which answers a
  narrower question.

- **`set_metadata("product", ...)`**: what the integrating program calls
  itself, e.g. `"mcnexus"` or the plugin's own name. An activation is shared by
  every program on a machine holding the same entitlement, and each one syncs;
  without a name attached, the version each reports overwrites the last, and
  the record answers no question reliably. `"product"` was already an accepted
  key that the handler ignored, so this adds no ABI surface. A program that
  declares nothing still works, and is recorded as unknown.

- **Operating system version** metadata is collected and sent on macOS and
  Windows. Reported, never enforced: it exists so activations on different
  systems can be distinguished without storing personal information. A machine
  that will not answer omits the field.

## [0.3.0]

**Every existing activation must be redone.** No API changed and 0.2.x code
recompiles untouched, but the machine binding this SDK computes is different,
so a certificate issued to 0.2.x reports `DEVICE_MISMATCH` under 0.3.0. Read
the migration note below before upgrading: the server cannot repair this on
its own.

### Fixed

- **Two builds of this SDK could disagree about what machine they were
  running on.** Prior to 0.3.0, `machineBinding` could diverge between builds
  linking different crypto backends, causing one binary on the workstation to
  activate and another binary to report `DEVICE_MISMATCH` on unchanged hardware.

  The machine binding computation is now standardized identically across all
  supported build configurations and backends, pinned by known-answer tests so
  all binaries on a machine always compute the exact same binding.

### Added

- `fingerprintVersion` now travels with every activation and sync request,
  and is stored against the activation (`2` for the standardized binding;
  `1` for legacy pre-0.3.0 clients). Requests that omit it are still accepted
  and recorded as `1`.

### Migrating from 0.2.x

The binding changes for every machine, and the server **cannot** map an old
binding to a new one; it only ever receives the hash, never the fingerprint
behind it. So a machine that re-activates looks like a new machine and
consumes another seat, which on a single-seat license means it is refused.

1. Upgrade **every** program on a given machine at once. A host application
   and a plugin that disagree about the SDK version will disagree about the
   machine, which is the same failure this release fixes.
2. Release the old activations for the affected licenses (they are visible as
   `fingerprintVersion = 1`).
3. Re-activate.

## [0.2.3]

No API changed, so 0.2.2 code recompiles against this header untouched.

### Fixed

- **A machine whose clock trailed the server rejected the certificate it had
  just activated with.** The issuer stamps `notBefore` from its own clock at
  the moment it signs; previously, the SDK compared that against the local
  clock with zero tolerance, so hosts slightly behind server time (common on
  isolated studio and render-farm networks without NTP) could reject fresh
  certificates immediately after activation. Verification now applies a bounded
  startup clock-skew tolerance on `notBefore`, kept strictly separate from
  clock-rollback detection; `expiresAt` and `offlineValidUntil` remain strict,
  so no license runs beyond its issued validity window.

### Note for render farms and other headless nodes

A headless node does local verification only and never starts the background
poller, so its receipt is never renewed and it stops being valid once the
offline window closes, with nothing wrong on the machine or the server. This
is existing behavior, not a change, but it is easy to be surprised by: a node
activated once and left running will deny after the grace period configured
for that license. Until file-based renewal exists, size the offline grace to
outlast the longest gap between re-activations.

## [0.2.2]

macOS only. No API changed, so 0.2.1 code recompiles against this header
untouched.

### Fixed

- **A stalled network call could hang forever on macOS.** After this SDK's
  own timeout elapsed and it cancelled the in-flight request, it waited a
  second time (with no timeout) for NSURLSession to confirm the
  cancellation. That confirmation is not contractually guaranteed to arrive
  promptly; Apple's own developer forums document cases where it does not
  arrive at all. When that happened, the calling thread blocked with no
  bound, reachable through `nexkeyruntime_license_destroy()`, meaning a host
  unloading the plugin at the wrong moment could hang on it. Every HTTP call
  this SDK makes on macOS shares the same transport, so this affected update
  checks, `activate`/`deactivate`/`sync`, and the background poller alike.
  Fixed to give up and return once its own timeout is reached, regardless of
  whether the OS ever confirms the cancellation (verified via ASan across a
  real network call that the object is destroyed at that point without
  triggering a later, dangling write). Windows was never affected: it enforces
  timeouts at the OS level with no equivalent wait.

## [0.2.1]

Bug fixes only. No API changed, so 0.2.0 code recompiles against this header
untouched, but every fix here is one you would rather have.

### Fixed

- **A stale license key could delete a valid receipt.** The background poller
  captured its request when it started, so deactivating one license and
  activating another left it syncing the key it began with. The server
  correctly answered `activation_removed` for that key (a true verdict about
  the wrong license), and the SDK then deleted *every* receipt in the tenant
  directory rather than the one it was told about. Activating a new key and
  forcing a sync could therefore destroy the license you had just installed.
  The request is now rebuilt each cycle, an absent stored key means "do not
  sync" instead of "sync with nothing", and only the affected receipt is
  removed.
- **`last_synced_at` was declared and never written.** The field is in
  `NexKeyRuntimeLicenseSnapshot` documented as "0 until a sync has happened",
  and nothing ever set it, so it stayed 0 forever for every caller. It now
  carries the last time a call actually reached the server, preferring the
  server's clock over the local one.
- **macOS: storage failed under a non-standard `$HOME`.** `~/Library` was
  missing from the directory chain and `mkdir` does not create parents, so a
  process whose home lacked it got an unusable storage path. Invisible on a
  normal install; it broke sandboxed and test environments.

## [0.2.0]

### Added

- `nexkeyruntime_license_set_callback`: subscribe to background sync
  results. Invoked on the poller thread whenever a sync completes, so a host
  learns about a suspension or revocation without polling the SDK itself.
  The callback must not block and must not call back into the handle.

### Changed

- Profile B is implemented. `nexkeyruntime_license_set_license_key`,
  `_set_metadata`, `_activate`, `_deactivate`, `_publish_receipt`,
  `_request_sync` and `_set_headless` no longer return
  `NEXKEYRUNTIME_NOT_AVAILABLE`; they perform activation, deactivation and
  background synchronization against the licensing backend. No existing
  function changed signature or behavior, so Profile A integrations are
  unaffected.
- `nexkeyruntime_license_destroy` now stops and joins the background sync
  thread before returning. This matters to plugin hosts: the call guarantees
  that no background SDK thread remains running when your bundle is unloaded.

### Added (scaffold, unchanged)

- Initial public scaffold: `LICENSE`/`NOTICE` (Apache-2.0), `README.md`,
  `CONTRIBUTING.md`, `SECURITY.md`, a draft `BINARY_LICENSE.md`, the public
  C header (`include/nexkeyruntime/nexkeyruntime.h`), integration and ABI
  policy docs, JSON Schemas for the activation certificate and ProductData
  formats, and a minimal non-OFX C example.

This entry marks the first public-repository release of the scaffold, not
the SDK's overall history: NexKeyRuntime itself has already shipped
offline licensing (Profile A) in the private monorepo; this repository is
catching up to that state, not starting from zero.
