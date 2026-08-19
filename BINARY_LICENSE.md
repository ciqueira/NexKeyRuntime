# NexKeyRuntime Binary License

> **DRAFT — pending legal/business review. Do not treat this as a binding
> license until explicitly approved.** It exists to give shape to the terms
> a compiled-binary release will need, not to be published as-is.

Last updated: August 18, 2026 (draft).

Copyright (c) 2026 Magno Ciqueira. All rights reserved.

This license is intended to apply only to official pre-compiled binary
releases of NexKeyRuntime (static libraries and headers distributed through
GitHub Releases for this repository). It is **not** the license for this
repository's own content — the header, schemas, docs, and examples here are
already Apache-2.0 (see [LICENSE](LICENSE)) and freely usable today. This
draft only governs the compiled artifact a third-party plugin developer
would statically link into their own product.

That is a different relationship than `MCPlugins/corePlugins/ColorEqualizer/
BINARY_LICENSE.md`, which is an end-user EULA for a finished, compiled
plugin. This document instead needs to grant a **developer redistribution
right**: permission to link the binary into a plugin the developer then
ships to their own end users.

## Open questions for legal/business review

- Is the binary free to redistribute for any developer, or gated behind a
  commercial agreement (per-tenant, per-seat, revenue share)?
- Does static linking require attribution in the licensee's own product, or
  is silent inclusion acceptable?
- What warranty disclaimer and liability limit apply to a third party whose
  own product depends on this binary functioning correctly?
- Support/SLA commitments, if any, for third-party integrators.

## Draft terms (placeholder shape)

### 1. Scope

Applies only to compiled NexKeyRuntime binaries obtained through an official
GitHub Release of this repository. Source code in this repository remains
governed by `LICENSE` (Apache-2.0).

### 2. License Grant

Subject to these terms[, and to a commercial agreement where required —
**TBD**], you are granted a limited, non-exclusive[, non-transferable]
license to statically link the compiled binary into your own plugin or
application and distribute the resulting compiled product to your end
users.

### 3. Restrictions

You may not:

- redistribute the NexKeyRuntime binary itself, standalone or repackaged,
  outside of a product you have compiled it into;
- reverse engineer, decompile, or disassemble the binary except to the
  extent applicable law expressly permits;
- remove or alter copyright or license notices embedded in the binary or
  its accompanying files;
- use the NexKeyRuntime name or trademarks to imply endorsement of your own
  product without permission.

### 4. Ownership

The binary is licensed, not sold. The copyright holder retains all rights,
title, and interest not expressly granted by this license.

### 5. Disclaimer of Warranty

THE BINARY IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED — **TBD, align with final legal review.**

### 6. Governing Law

**TBD.**

### 7. Contact

Licensing questions: **TBD — add a public contact channel before this
document leaves draft status.**
