# Security Policy

## Reporting a vulnerability

Do not open a public issue for a suspected vulnerability. Report it
privately through GitHub Security Advisories for this repository.

Include the affected version, reproduction steps, impact, and any suggested
mitigation. Please avoid accessing data that does not belong to you.

## Scope

This repository ships the public contract only (headers, schemas, docs,
examples) — it must never contain license keys, backend credentials,
protected download URLs, tenant secrets, or signing keys. If you find one
committed here, report it as a vulnerability, not a bug.

Implementation-level reports relevant to consumers of this SDK are also in
scope even though the code lives in a private monorepo, in particular:

- a way to bypass Ed25519 certificate or ProductData signature verification;
- a way to make `nexkeyruntime_license_render_decision` return ALLOW without
  a valid, unexpired, non-revoked local receipt;
- memory-safety issues reachable across the C ABI boundary (the functions
  declared in `include/nexkeyruntime/nexkeyruntime.h`).
