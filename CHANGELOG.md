# Changelog

All notable changes to the NexKeyRuntime public repository will be
documented in this file.

## [0.4.0]

No API changed and no stored state is affected, so 0.3.0 code recompiles
untouched and activations made against it keep working. What changes is what
the SDK reports about itself and the machine.

### Added

- **The SDK now reports its own version on every activation and sync.** It was
  the one participant that could not be wrong about it — no integrator has to
  remember a call, and no build can claim a version it is not — yet it was the
  one thing never sent. Until now the only way to tell which SDK produced an
  activation was to infer it from `fingerprintVersion`, which answers a
  narrower question.

- **`set_metadata("product", ...)`** — what the integrating program calls
  itself, e.g. `"mcnexus"` or the plugin's own name. An activation is shared by
  every program on a machine holding the same entitlement, and each one syncs;
  without a name attached, the version each reports overwrites the last, and
  the record answers no question reliably. `"product"` was already an accepted
  key that the handler ignored, so this adds no ABI surface. A program that
  declares nothing still works, and is recorded as unknown.

- **Operating system version** is collected and sent: `kern.osproductversion`
  on macOS, `RtlGetVersion` on Windows. Reported, never enforced — it exists so
  an activation can be told apart from another of the same customer's without
  anything personal being stored. A machine that will not answer omits the
  field.

## [0.3.0]

**Every existing activation must be redone.** No API changed and 0.2.x code
recompiles untouched, but the machine binding this SDK computes is different,
so a certificate issued to 0.2.x reports `DEVICE_MISMATCH` under 0.3.0. Read
the migration note below before upgrading — the server cannot repair this on
its own.

### Fixed

- **Two builds of this SDK could disagree about what machine they were
  running on.** The 256-bit hash behind `machineBinding` was deliberately
  *not* the same algorithm on every backend: SHA-256 when built against
  libsodium, BLAKE2b-256 when built against the vendored Monocypher. The
  reasoning was that the value never left the process — but it does. It is
  sent to the gateway at activation, signed into the certificate, and then
  recomputed and compared by a *different binary* on the same machine: one
  program activates, another verifies.

  Where both were built the same way, nothing went wrong, which is why this
  survived: release archives force Monocypher, so everything published agreed
  with everything else. The moment one consumer was built on a machine with
  libsodium installed, it computed a binding no other build recognised, and
  the failure surfaced as `DEVICE_MISMATCH` on hardware that had not changed
  — the least informative symptom possible for the actual cause.

  `localHash256` is now **SHA-512 truncated to the leading 32 bytes** on every
  backend, and a known-answer test pins it so the two can never drift apart
  again silently. SHA-512 was chosen over the alternatives because it is
  present in both backends already and reproducible in every language and
  runtime — including Node and WebCrypto, where BLAKE2b-256 is simply not
  available and cannot be derived by truncating BLAKE2b-512.

### Added

- `fingerprintVersion` now travels with every activation and sync request,
  and is stored against the activation. It is `2` for the binding described
  above; `1` means a pre-0.3.0 client, whose recipe depended on how it was
  built. Requests that omit it are still accepted and recorded as `1`.

### Migrating from 0.2.x

The binding changes for every machine, and the server **cannot** map an old
binding to a new one — it only ever receives the hash, never the fingerprint
behind it. So a machine that re-activates looks like a new machine and
consumes another seat, which on a single-seat licence means it is refused.

1. Upgrade **every** program on a given machine at once. A host application
   and a plugin that disagree about the SDK version will disagree about the
   machine, which is the same failure this release fixes.
2. Release the old activations for the affected licences (they are visible as
   `fingerprintVersion = 1`).
3. Re-activate.

## [0.2.3]

No API changed, so 0.2.2 code recompiles against this header untouched.

### Fixed

- **A machine whose clock trailed the server rejected the certificate it had
  just activated with.** The issuer stamps `notBefore` from its own clock at
  the moment it signs; the SDK compared that against the local clock with no
  tolerance, so any machine running even a second or two behind the gateway
  failed verification with "certificate is not yet valid" — at activation,
  the one moment the certificate is guaranteed to be fresh. Network latency
  masked it whenever the round trip exceeded the drift, which is why it
  survived: it reproduces on drifted clocks and on fast connections, not in
  ordinary testing. Machines with NTP blocked by a firewall are common in the
  isolated studio and render-farm networks this SDK is built for, and those
  are exactly the machines that drift. Now given the same 60-second leeway a
  JWT library applies to `nbf`, tracked in a field of its own rather than
  folded into the clock-rollback tolerance — rollback is fraud to detect,
  skew is drift to tolerate, and conflating them would let one be widened for
  the other's reasons. Only the *start* of the validity window is forgiving:
  `expiresAt` and `offlineValidUntil` remain strict, so no licence runs a
  second longer than it was issued for.

### Note for render farms and other headless nodes

A headless node does local verification only and never starts the background
poller, so its receipt is never renewed and it stops being valid once the
offline window closes — with nothing wrong on the machine or the server. This
is existing behaviour, not a change, but it is easy to be surprised by: a node
activated once and left running will deny after the grace period configured
for that licence. Until file-based renewal exists, size the offline grace to
outlast the longest gap between re-activations.

## [0.2.2]

macOS only. No API changed, so 0.2.1 code recompiles against this header
untouched.

### Fixed

- **A stalled network call could hang forever on macOS.** After this SDK's
  own timeout elapsed and it cancelled the in-flight request, it waited a
  second time — with no timeout — for NSURLSession to confirm the
  cancellation. That confirmation is not contractually guaranteed to arrive
  promptly; Apple's own developer forums document cases where it does not
  arrive at all. When that happened, the calling thread blocked with no
  bound — reachable through `nexkeyruntime_license_destroy()`, meaning a host
  unloading the plugin at the wrong moment could hang on it. Every HTTP call
  this SDK makes on macOS shares the same transport, so this affected update
  checks, `activate`/`deactivate`/`sync`, and the background poller alike.
  Fixed to give up and return once its own timeout is reached, regardless of
  whether the OS ever confirms the cancellation — verified via ASan across a
  real network call that the object is destroyed at that point without
  triggering a later, dangling write. Windows was never affected: it enforces
  timeouts at the OS level with no equivalent wait.

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
