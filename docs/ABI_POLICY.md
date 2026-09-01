# ABI policy

NexKeyRuntime is a C ABI, C++14-implemented static library. This document
describes the compatibility guarantees the header already encodes today —
it adds no new function or type; it makes the existing conventions
explicit for integrators who need to reason about upgrade safety.

## Versioning

`NEXKEYRUNTIME_VERSION_MAJOR` / `_MINOR` / `_PATCH` and
`NEXKEYRUNTIME_VERSION_STRING` are compile-time macros; `nexkeyruntime_
version()` returns the same string at runtime, so a host can confirm which
build it actually linked against, not just which headers it compiled with.
While `0.x`, the public API may still evolve — see `CHANGELOG.md` for what
changed and when.

## Result codes never get renumbered

`NexKeyRuntimeResult` is append-only by explicit convention (see the
comment directly above the licensing block in the header): values 0–10
shipped with the update/notice handle; licensing values start at 20 and are
grouped by concern — 20s are ProductData/config errors, 30s are
server-reported license errors (Profile B), 40s are
local-proof errors from `load_local`, 50s are environment errors. A future
release may add a new value in an existing block or open a new one; it will
never reuse or renumber an existing one. Treat unrecognized values as a
generic failure, not as a crash — switch statements should always have a
default case.

`NEXKEYRUNTIME_E_ABI_MISMATCH` (22) is the runtime signal for "this build
does not understand this ProductData's `formatVersion`" — the SDK already
has forward-compatibility signaling for its own primary data format; there
is no separate need for callers to introspect a numeric ABI version before
calling into it.

## Struct layout

`NexKeyRuntimeConfig`, `NexKeyRuntimeLicenseSnapshot`, and similar
plain structs use fixed-capacity `char[]` buffers
(`NEXKEYRUNTIME_ID_CAPACITY`, `_TITLE_CAPACITY`, `_MESSAGE_CAPACITY`,
`_URL_CAPACITY`) rather than pointers or dynamically sized fields — no
allocator crosses the ABI boundary. New fields are only ever appended at
the end of a struct, never inserted or reordered, and `struct_size` (present
on the snapshot structs) lets a caller detect at runtime whether it is
looking at a struct larger than the one it was compiled against.

## Opaque handles

`NexKeyRuntimeHandle` and `NexKeyRuntimeLicenseHandle` are forward-declared,
incomplete types. Callers only ever hold a pointer, obtained from
`_create()` and released via `_destroy()` — internal layout can change
between releases without breaking a compiled caller, because no caller ever
dereferences one directly.

## Linkage

Every declaration is wrapped in `extern "C"` (`#ifdef __cplusplus`), so the
symbol names are stable C symbols regardless of which C++ compiler or
standard library produced them — a prerequisite for linking a prebuilt
static library against a host built with a different toolchain.
