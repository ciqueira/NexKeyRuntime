# Changelog

All notable changes to the NexKeyRuntime public repository will be
documented in this file.

## [0.2.1]

Bug fixes only. No API changed, so 0.2.0 code recompiles against this header
untouched — but every fix here is one you would rather have.

### Fixed

- **A stale license key could delete a valid receipt.** The background poller
  captured its request when it started, so deactivating one license and
  activating another left it syncing the key it began with. The server
  correctly answered `activation_removed` for that key — a true verdict about
  the wrong license — and the SDK then deleted *every* receipt in the tenant
  directory rather than the one it was told about. Activating a new key and
  forcing a sync could therefore destroy the licence you had just installed.
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

- `nexkeyruntime_license_set_callback` — subscribe to background sync
  results. Invoked on the poller thread whenever a sync completes, so a host
  learns about a suspension or revocation without polling the SDK itself.
  The callback must not block and must not call back into the handle.

### Changed

- Perfil B is implemented. `nexkeyruntime_license_set_license_key`,
  `_set_metadata`, `_activate`, `_deactivate`, `_publish_receipt`,
  `_request_sync` and `_set_headless` no longer return
  `NEXKEYRUNTIME_NOT_AVAILABLE`; they perform activation, deactivation and
  background synchronisation against the licensing backend. No existing
  function changed signature or behaviour, so Perfil A integrations are
  unaffected.
- `nexkeyruntime_license_destroy` now stops and joins the background sync
  thread before returning. This matters to plugin hosts: the call is the
  guarantee that no thread of ours is still running when your bundle is
  unloaded.

### Added (scaffold, unchanged)

- Initial public scaffold: `LICENSE`/`NOTICE` (Apache-2.0), `README.md`,
  `CONTRIBUTING.md`, `SECURITY.md`, a draft `BINARY_LICENSE.md`, the public
  C header (`include/nexkeyruntime/nexkeyruntime.h`), integration and ABI
  policy docs, JSON Schemas for the activation certificate and ProductData
  formats, and a minimal non-OFX C example.

This entry marks the first public-repository release of the scaffold, not
the SDK's overall history — NexKeyRuntime itself has already shipped
offline licensing (Perfil A) in the private monorepo; this repository is
catching up to that state, not starting from zero.
