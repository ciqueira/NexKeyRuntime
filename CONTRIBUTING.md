# Contributing

This repository ships the public contract for NexKeyRuntime: the C header,
protocol schemas, and consumer documentation and examples. It does not
contain the license and update engine's implementation — that lives in a
private monorepo. This changes what a contribution here can be:

- fixes and clarifications to `README.md`, `docs/`, and the examples;
- corrections to `schemas/*.schema.json` when they drift from what the
  header or a real certificate/ProductData blob actually produces;
- new non-OFX example code demonstrating the public API.

Before submitting:

1. do not add tenant secrets, protected URLs, backend code, license keys, or
   internal roadmaps;
2. keep `include/nexkeyruntime/nexkeyruntime.h` byte-identical to the
   private source of truth — CI rejects a divergent copy; do not hand-edit
   it here;
3. document public-facing changes in `CHANGELOG.md`.

A change to the ABI itself (a new function, a changed struct, a new enum
value) cannot be made in this repository. Open an issue describing the need
— the actual header edit happens in the private monorepo and is mirrored
here afterward.

Unless explicitly stated otherwise, intentionally submitted contributions
are licensed under Apache-2.0.
