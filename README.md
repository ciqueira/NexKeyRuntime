# NexKeyRuntime

NexKeyRuntime is a native C/C++14 SDK for update discovery, product notices,
and offline license verification in desktop plugins and applications backed by
Nexus.

It exposes two independent handles:

- `NexKeyRuntimeHandle`: update checks and product notices (both profiles,
  always available).
- `NexKeyRuntimeLicenseHandle`: offline license verification and self-service
  activation.

---

## Developer Sandbox & Quickstart

This repository includes a dedicated **[Developer Sandbox](sandbox/)** with pre-provisioned test license keys, signed trust envelopes, and a ready-to-run test environment:

- **[sandbox/](sandbox/)**: Dedicated Sandbox documentation, trust envelope (`mcnexus-sandbox.nexkeydat`), test keys catalog (`licenses.json`), and the **[all-in-one OFX reference plugin](sandbox/ofx-plugin/)**.
- **[tools/nexkeyctl/](tools/nexkeyctl/)**: The official command-line utility to activate, deactivate, inspect license state, and test updates/notices from your terminal.
- **[examples/](examples/)**: Minimal, single-purpose C examples for every SDK feature.

### Build All Examples and CLI

```bash
# Build the Clean Reference OFX Plugin (Profile A Showcase):
cmake -S sandbox/ofx-plugin -B sandbox/ofx-plugin/build
cmake --build sandbox/ofx-plugin/build -j8

# Build all minimal SDK examples:
cmake -S examples -B examples/build
cmake --build examples/build -j8

# Build the CLI utility:
cmake -S tools/nexkeyctl -B tools/nexkeyctl/build
cmake --build tools/nexkeyctl/build -j8
```

---

## Profile A vs. Profile B

- **Profile A**: the host application (MCNexus App or `nexkeyctl`) activates the license; the
  plugin only loads a ProductData blob and a locally cached receipt, then makes
  a fast, read-only ALLOW/DENY decision on the render thread (`< 1 ns` atomic check).
  *See [sandbox/ofx-plugin](sandbox/ofx-plugin) for the complete reference OFX plugin with Inspector controls.*
- **Profile B**: the product itself activates and syncs a license, with no host
  application in the loop. Functions
  `nexkeyruntime_license_activate`, `_deactivate`, `_request_sync`, and
  `_publish_receipt` perform real activation, deactivation, and background
  synchronization against the licensing backend.
  *See [examples/02-activation](examples/02-activation) for a minimal autonomous client.*

Both profiles work, and the public API is stable as of 1.0.0 (see
[docs/ABI_POLICY.md](docs/ABI_POLICY.md) for the compatibility guarantees).
Third-party use of the compiled binaries is licensed under
[BINARY_LICENSE.md](BINARY_LICENSE.md).

---

## Minimal Integration (Profile A)

```c
#include <nexkeyruntime/nexkeyruntime.h>

static NexKeyRuntimeLicenseHandle *g_license = NULL;

// Injected from sandbox/mcnexus-sandbox.nexkeydat or issued by the backoffice.
// Contains only a public keyring and a base URL (no secret).
static const char *NEXKEYRUNTIME_PRODUCT_DATA = "...";

void pluginLoad(void) {
    g_license = nexkeyruntime_license_create();
    nexkeyruntime_license_set_product_data(g_license, NEXKEYRUNTIME_PRODUCT_DATA);
    nexkeyruntime_license_set_tenant_id(g_license, "mcnexus-sandbox");
    nexkeyruntime_license_set_variant(g_license, "default");
    nexkeyruntime_license_load_local(g_license);
}

// Render thread: no network, no file I/O, no JSON parsing; a single atomic read.
int render(void) {
    return nexkeyruntime_license_render_decision(g_license) == NEXKEYRUNTIME_RENDER_ALLOW;
}

void pluginUnload(void) {
    nexkeyruntime_license_destroy(g_license);
    g_license = NULL;
}
```

For C++ OFX plugins, [`include/MCLicense.h`](include/MCLicense.h) provides a zero-overhead convenience wrapper handling thread-safe receipt reloading and telemetry reports.

---

## Documentation

- [sandbox/](sandbox/): sandbox environment, pre-provisioned keys, and AI prompts.
- [docs/INTEGRATION.md](docs/INTEGRATION.md): the license handle end to end (both profiles, UI status reporting, and how external activations reach your plugin).
- [docs/UPDATES_AND_NOTICES.md](docs/UPDATES_AND_NOTICES.md): the update handle (release checks, product notices, audience and the actions you can offer).
- [docs/OFFLINE.md](docs/OFFLINE.md): activating a machine that cannot reach the network, and releasing its seat afterwards.
- [docs/ABI_POLICY.md](docs/ABI_POLICY.md): ABI stability and compatibility guarantees.
- [sandbox/ofx-plugin/](sandbox/ofx-plugin/): all-in-one OpenFX reference plugin for DaVinci Resolve.
- [examples/](examples/): minimal single-purpose C examples:
  - `01-minimal`: fast terminal validator (< 1 ns atomic check)
  - `02-activation`: autonomous licensing
  - `03-updates`: release checks and channels
  - `04-notices`: host-targeted product advisories
  - `05-offline`: air-gapped activation workflow
- [schemas/](schemas/): JSON wire formats: ProductData, activation certificate, and update manifest.

---

## Windows: Link with the Static CRT (`/MT`)

The published `nexkeyruntime.lib` is built against the **static** C runtime,
so anything you link it into must use the same one:

```cmake
# CMake: CMP0091 is NEW from 3.15, and its default is the dynamic CRT.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
```

If your build also compiles CUDA, pass `-Xcompiler /MT` to `nvcc`.

---

## License

The contents of this repository (headers, schemas, docs, examples) are
licensed under the Apache License, Version 2.0; see [LICENSE](LICENSE).
Compiled binary releases are governed separately by
[BINARY_LICENSE.md](BINARY_LICENSE.md).
