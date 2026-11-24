# nexkeyctl: NexKeyRuntime Command-Line Control Tool

`nexkeyctl` is the native CLI utility for:
1. **Activating, inspecting, and managing NexKeyRuntime licenses** (`--activate`, `--deactivate`, `--status`, `--sync`, `--export-request`, `--import`, `--export-deactivation`).
2. **Checking remote updates and product notices** (`--updates`).
3. **Packaging DCTL shaders into secure OpenFX plugins via Asset Vault** (`package-dctl`, `generate-asset-keys`).

All commands below assume your working directory is the **repository root** (`NexKeyRuntime/`).

---

## 1. Building `nexkeyctl` from Source

Compiling `nexkeyctl` on your machine ensures the executable is locally produced and recognized as trusted by macOS (avoiding Gatekeeper quarantine alerts).

### Prerequisites
- **Compiler**: C++17 compliant compiler (AppleClang 14+, GCC 10+, or MSVC 2019+).
- **CMake**: Version 3.20 or newer.
- **Static Libraries**: Download the **NexKeyRuntime SDK** release package for your platform from [GitHub Releases](https://github.com/ciqueira/NexKeyRuntime/releases) and place `lib/libnexkeyruntime.a` and `lib/libnexkeypackager.a` (macOS) or `lib/nexkeyruntime.lib` and `lib/nexkeypackager.lib` (Windows) in `<repo>/lib/`.

### Recommended: Unified Root Build

**macOS:**
```bash
cmake -S . -B build
cmake --build build -j8
```
*Artifact output:* `build/bin/nexkeyctl`

**Windows (PowerShell / Developer Command Prompt):**
```cmd
cmake -S . -B build -A x64
cmake --build build --config Release
```
*(Use `-A ARM64` instead of `-A x64` if you downloaded `windows-arm64.zip`; `-A` must match your downloaded release archive architecture.)*
*Artifact output:* `build\bin\Release\nexkeyctl.exe`

### Optional: Standalone CLI Build
If you only want to compile `nexkeyctl` without the examples:
```bash
cmake -S tools/nexkeyctl -B tools/nexkeyctl/build
cmake --build tools/nexkeyctl/build -j8
```
*(Pass `-DCMAKE_PREFIX_PATH=/path/to/nexkeyruntime-sdk` if the library is outside `<repo>/lib/`.)*

---

## 2. License Management & Update Commands

`nexkeyctl` defaults `--tenant` to `mcnexus-sandbox` and automatically locates `sandbox/mcnexus-sandbox.nexkeydat` when run inside this repository. For clarity, the full explicit flags are shown below:

**macOS:**
```bash
# 1. Activate a license:
./build/bin/nexkeyctl --activate \
  --key=2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08 \
  --tenant=mcnexus-sandbox \
  --product-file=sandbox/mcnexus-sandbox.nexkeydat

# 2. Check local license receipt status and resolved path:
./build/bin/nexkeyctl --status \
  --tenant=mcnexus-sandbox \
  --product-file=sandbox/mcnexus-sandbox.nexkeydat

# 3. Synchronize license with the gateway:
./build/bin/nexkeyctl --sync \
  --tenant=mcnexus-sandbox \
  --product-file=sandbox/mcnexus-sandbox.nexkeydat

# 4. Deactivate the license and release the seat:
./build/bin/nexkeyctl --deactivate \
  --tenant=mcnexus-sandbox \
  --product-file=sandbox/mcnexus-sandbox.nexkeydat

# 5. Check for updates and vendor notices:
./build/bin/nexkeyctl --updates \
  --current-version=1.0.0 \
  --tenant=mcnexus-sandbox \
  --base-url=https://sdk-sandbox.mcnexus.app \
  --host-name=DaVinciResolve

# 6. Offline air-gapped activation workflow (see docs/OFFLINE.md):
./build/bin/nexkeyctl --export-request --out=machine-request.json --tenant=mcnexus-sandbox --product-file=sandbox/mcnexus-sandbox.nexkeydat
./build/bin/nexkeyctl --import --response=activation-response.json --tenant=mcnexus-sandbox --product-file=sandbox/mcnexus-sandbox.nexkeydat
./build/bin/nexkeyctl --export-deactivation --out=deactivation-proof.json --tenant=mcnexus-sandbox --product-file=sandbox/mcnexus-sandbox.nexkeydat
```

**Windows:** Replace `./build/bin/nexkeyctl` with `.\build\bin\Release\nexkeyctl.exe`.

---

## 3. DCTL-to-OFX Conversion Commands

All packaged bundles default to `dist/` at the repository root (`--output=dist`). In `--sandbox` mode, `nexkeyctl` also auto-detects the platform runner if `--runner` is omitted.

### Sandbox / Evaluation Mode (Free, Instant)
Convert a DaVinci Resolve DCTL script into an OpenFX bundle inside `dist/`:

**macOS:**
```bash
./build/bin/nexkeyctl package-dctl \
  --input=sandbox/dctl-to-ofx/examples/01-exposure/Exposure.dctl \
  --name="Exposure Simple" \
  --runner=sandbox/dctl-to-ofx/runners/macos/NexDCTLRunner.ofx \
  --output=dist \
  --sandbox
```

**Windows (PowerShell / CMD):**
```cmd
.\build\bin\Release\nexkeyctl.exe package-dctl --input=sandbox\dctl-to-ofx\examples\01-exposure\Exposure.dctl --name="Exposure Simple" --runner=sandbox\dctl-to-ofx\runners\windows\NexDCTLRunner.ofx --output=dist --sandbox
```

### Production Mode (Commercial Licensing & Asset Vault Protection)
Pack encrypted, signed payloads bound to your product credentials using the NexKeyRuntime Asset Vault:
```bash
# 1. Generate cryptographic keys:
./build/bin/nexkeyctl generate-asset-keys --product-id=com.studio.look --output=./keys

# 2. Package commercial plugin bundle into dist/:
./build/bin/nexkeyctl package-dctl \
  --input=FilmLook.dctl \
  --product-id=com.studio.look \
  --keys=./keys/com.studio.look.keys \
  --runner=sandbox/dctl-to-ofx/runners/macos/NexDCTLRunner.ofx \
  --nexkeydat=product.nexkeydat \
  --tenant=studio-tenant \
  --variant=download:standard \
  --output=dist
```

---

## 4. Sandboxed Host Diagnostics

When running inside DaVinci Resolve or host environments installed from the Mac App Store or Windows MSIX AppContainer, application storage paths may be sandboxed and redirected.

`nexkeyctl --status` explicitly prints the exact filesystem path resolved for receipts so you can verify that external activations align with the directory accessed by the plugin inside the host.
