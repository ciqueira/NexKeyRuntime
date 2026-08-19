# Changelog

All notable changes to the NexKeyRuntime public repository will be
documented in this file.

## [0.2.0] — prepared, not yet released

> The published binary is still `v0.1.0`, built before Perfil B existed. The
> header on `main` declares functions that release does not export — build
> against the header inside the release archive, not this one, until `v0.2.0`
> is tagged.
>
> The minor bump (not a patch) is why: this adds a public function and turns
> seven declared-but-inert ones into working API. Nothing already shipped
> changed signature or behaviour.

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
