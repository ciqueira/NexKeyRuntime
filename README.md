# NexKeyRuntime

NexKeyRuntime is a native C/C++ SDK for update discovery, product notices,
offline license verification, and real-time MIDI hardware control (NexControl)
in desktop plugins and applications backed by Nexus.

It exposes three primary capabilities:

- `NexKeyRuntimeHandle`: update checks and product notices (both profiles,
  always available).
- `NexKeyRuntimeLicenseHandle`: offline license verification and self-service
  activation.
- `NexControl`: zero-latency binary IPC subsystem connecting OpenFX plugins
  with physical MIDI hardware surfaces (such as Midi Fighter Twister and custom MIDI controllers).

---

## Developer Sandbox & Components

This repository includes a dedicated **[Developer Sandbox](sandbox/)** with pre-provisioned test license keys, signed trust envelopes, precompiled OFX runners, and a ready-to-run test environment:

- **[sandbox/dctl-to-ofx/](sandbox/dctl-to-ofx/)**: Convert DCTL color scripts and 3D LUTs into native OpenFX plugins with zero C++ code and Asset Vault encryption using precompiled macOS (Metal) and Windows (OpenCL) runners.
- **[sandbox/](sandbox/)**: Dedicated Sandbox documentation, trust envelope (`mcnexus-sandbox.nexkeydat`), test keys catalog (`licenses.json`), and the **[reference OFX plugin](sandbox/ofx-plugin/)**.
- **[tools/nexkeyctl/](tools/nexkeyctl/)**: The official command-line utility to activate, deactivate, inspect license state, package DCTLs, and test updates/notices from your terminal.
- **[examples/](examples/)**: Minimal, single-purpose C/C++ examples for every SDK feature.

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

For C++ OFX plugins, [`include/MCLicense.h`](include/MCLicense.h) provides a zero-overhead convenience wrapper handling thread-safe receipt reloading and telemetry reports, and [`include/MCNexControl.h`](include/MCNexControl.h) provides zero-latency hardware parameter binding.

---

## Documentation

- [sandbox/](sandbox/): sandbox environment, pre-provisioned keys, and AI prompts.
- [docs/INTEGRATION.md](docs/INTEGRATION.md): the license handle end to end (both profiles, UI status reporting, and how external activations reach your plugin).
- [docs/NEXCONTROL_INTEGRATION.md](docs/NEXCONTROL_INTEGRATION.md): integrating the real-time MIDI hardware control subsystem (NexControl) into OpenFX plugins.
- [docs/UPDATES_AND_NOTICES.md](docs/UPDATES_AND_NOTICES.md): the update handle (release checks, product notices, audience and the actions you can offer).
- [docs/OFFLINE.md](docs/OFFLINE.md): activating a machine that cannot reach the network, and releasing its seat afterwards.
- [docs/ABI_POLICY.md](docs/ABI_POLICY.md): ABI stability and compatibility guarantees.
- [sandbox/ofx-plugin/](sandbox/ofx-plugin/): reference OpenFX plugin for DaVinci Resolve.
- [examples/](examples/): minimal single-purpose C/C++ examples:
  - `01-minimal`: fast terminal validator (< 1 ns atomic check)
  - `02-activation`: autonomous licensing
  - `03-updates`: release checks and channels
  - `04-notices`: host-targeted product advisories
  - `05-offline`: air-gapped activation workflow
  - `06-nexcontrol-ofx`: hardware MIDI control and parameter automation in OpenFX
- [schemas/](schemas/): JSON wire formats: ProductData, activation certificate, update manifest, and MIDI device profile.

---

## Quickstart: Manual First Steps

All commands across this repository are designed to be executed from the **repository root** (`NexKeyRuntime/`).

### 1. Clone the repository

```bash
git clone https://github.com/ciqueira/NexKeyRuntime.git
cd NexKeyRuntime
```

### 2. Place the precompiled SDK library in `lib/`

Download the precompiled release archive for your platform from [GitHub Releases](https://github.com/ciqueira/NexKeyRuntime/releases), extract it, and copy its `lib/` directory into the repository root:

- **macOS**: `lib/libnexkeyruntime.a` (plus optional `lib/libnexcontrol.a` for NexControl OFX MIDI examples)
- **Windows**: `lib/nexkeyruntime.lib` (plus optional `lib/nexcontrol.lib` for NexControl OFX MIDI examples)

*(Note: If you use `-DCMAKE_PREFIX_PATH=<extracted_release_dir>` on Windows instead of copying `lib/` into the repository root, keep the extracted `bin/` directory (`nexkeyruntime.dll`, `nexcontrol_hub.dll`) alongside `lib/` so CMake's exported package targets validate cleanly.)*

Your workspace layout will follow this standard structure:

```text
NexKeyRuntime/
├── lib/                       # Downloaded release static library (gitignored)
│   ├── libnexkeyruntime.a     # macOS (or nexkeyruntime.lib on Windows)
│   └── libnexcontrol.a        # Optional: only required for NexControl OFX MIDI examples
├── build/
│   ├── bin/                   # Compiled CLI and examples (build\bin\Release\ on Windows)
│   └── plugins/               # Compiled OpenFX bundles (when -DNEXKEY_BUILD_OFX=ON)
├── dist/                      # Packaged DCTL-to-OFX plugin bundles
├── include/                   # Public C/C++ headers
├── sandbox/                   # Test credentials, keys, DCTL runners, and reference OFX plugin
└── tools/nexkeyctl/           # CLI source
```

### 3. Build CLI & Examples from the repository root

**macOS:**
```bash
cmake -S . -B build
cmake --build build -j8
```

**Windows (PowerShell / Developer Command Prompt):**
```cmd
cmake -S . -B build -A x64
cmake --build build --config Release
```
*(Use `-A ARM64` instead of `-A x64` if you downloaded `windows-arm64.zip`; `-A` must match your downloaded release archive architecture.)*

*(Optional)* To also build the OpenFX reference plugin (`sandbox/ofx-plugin`, and `examples/06-nexcontrol-ofx` if `libnexcontrol` is in `lib/`):
```bash
# macOS:
cmake -S . -B build -DNEXKEY_BUILD_OFX=ON
cmake --build build -j8
```
```cmd
cmake -S . -B build -A x64 -DNEXKEY_BUILD_OFX=ON
cmake --build build --config Release
```
*(Pass `-DOFX_ROOT=/path/to/openfx` if you already have a local OpenFX SDK checkout; otherwise CMake fetches it automatically.)*

### 4. Verify your setup

Run these commands from the repository root (`NexKeyRuntime/`):

**macOS:**
```bash
# 1. Inspect local license state (auto-detects sandbox/mcnexus-sandbox.nexkeydat):
./build/bin/nexkeyctl --status --tenant=mcnexus-sandbox --product-file=sandbox/mcnexus-sandbox.nexkeydat

# 2. Activate a pre-provisioned Sandbox Demo license:
./build/bin/nexkeyctl --activate --key=2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08 --tenant=mcnexus-sandbox --product-file=sandbox/mcnexus-sandbox.nexkeydat

# 3. Validate Profile A offline render decision (< 1 ns check):
./build/bin/license-validator

# 4. Package a sample DCTL into a ready-to-load OpenFX bundle under dist/:
./build/bin/nexkeyctl package-dctl \
  --input=sandbox/dctl-to-ofx/examples/01-exposure/Exposure.dctl \
  --name="Exposure Simple" \
  --runner=sandbox/dctl-to-ofx/runners/macos/NexDCTLRunner.ofx \
  --output=dist \
  --sandbox
```

**Windows (PowerShell / CMD):**

1. Inspect local license state:
   ```cmd
   .\build\bin\Release\nexkeyctl.exe --status --tenant=mcnexus-sandbox --product-file=sandbox\mcnexus-sandbox.nexkeydat
   ```
2. Activate a pre-provisioned Sandbox Demo license:
   ```cmd
   .\build\bin\Release\nexkeyctl.exe --activate --key=2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08 --tenant=mcnexus-sandbox --product-file=sandbox\mcnexus-sandbox.nexkeydat
   ```
3. Validate Profile A offline render decision (`< 1 ns` check):
   ```cmd
   .\build\bin\Release\license-validator.exe
   ```
4. Package a sample DCTL into `dist\`:
   ```cmd
   .\build\bin\Release\nexkeyctl.exe package-dctl --input=sandbox\dctl-to-ofx\examples\01-exposure\Exposure.dctl --name="Exposure Simple" --runner=sandbox\dctl-to-ofx\runners\windows\NexDCTLRunner.ofx --output=dist --sandbox
   ```

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
