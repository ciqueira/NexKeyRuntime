# nexkeyctl: NexKeyRuntime Command-Line Control Tool

`nexkeyctl` is the native CLI utility for:
1. **Packaging DCTL shaders into secure OpenFX plugins** (`package-dctl`, `generate-asset-keys`).
2. **Activating, inspecting, and managing NexKeyRuntime licenses** (`--activate`, `--deactivate`, `--status`, `--sync`).
3. **Checking remote updates and product notices** (`--updates`).

---

## 1. Building `nexkeyctl` from Source

Compiling `nexkeyctl` on your machine ensures the executable is locally produced and recognized as trusted by macOS (avoiding Gatekeeper quarantine alerts).

### Prerequisites
- CMake 3.20+
- C++17 compiler (Clang / Xcode on macOS, MSVC on Windows)
- The **NexKeyRuntime SDK** binary package (downloaded and extracted from [GitHub Releases](https://github.com/ciqueira/NexKeyRuntime/releases))

### Build Steps

```bash
# Point CMAKE_PREFIX_PATH to the extracted NexKeyRuntime SDK directory:
cmake -S tools/nexkeyctl -B tools/nexkeyctl/build -DCMAKE_PREFIX_PATH=/path/to/nexkeyruntime-sdk

# Compile the executable:
cmake --build tools/nexkeyctl/build --config Release
```

*Binary executable location:*
- **macOS**: `tools/nexkeyctl/build/nexkeyctl`
- **Windows**: `tools/nexkeyctl/build/Release/nexkeyctl.exe`

---

## 2. Using Precompiled Binaries

Precompiled binaries for macOS (Universal `arm64` + `x86_64`) and Windows (`x64`, `arm64`, `x86`) are included in every release archive under `bin/`.

> [!NOTE]
> **macOS Gatekeeper**: If downloaded through a web browser (Safari/Chrome), macOS attaches a quarantine attribute to downloaded archives. Run the following command once to clear the quarantine flag before executing:
> ```bash
> xattr -dr com.apple.quarantine bin/nexkeyctl
> ```

---

## 3. DCTL-to-OFX Conversion Commands

### Sandbox / Evaluation Mode (Free, Instant)
Convert any standard DaVinci Resolve DCTL script into a loaded OFX bundle for testing:
```bash
./nexkeyctl package-dctl \
  --input=examples/01-exposure/Exposure.dctl \
  --sandbox \
  --runner=runners/macos/NexDCTLRunner.ofx \
  --output=build
```

### Production Mode (Commercial Licensing & Asset Protection)
Pack encrypted, tamper-proof payloads bound to your product credentials:
```bash
# 1. Generate cryptographic keys:
./nexkeyctl generate-asset-keys --product-id=com.studio.look --output=./keys

# 2. Package commercial plugin bundle:
./nexkeyctl package-dctl \
  --input=FilmLook.dctl \
  --product-id=com.studio.look \
  --keys=./keys/com.studio.look.keys \
  --runner=runners/macos/NexDCTLRunner.ofx \
  --nexkeydat=product.nexkeydat \
  --tenant=studio-tenant \
  --variant=download:standard \
  --output=dist
```

---

## 4. License Management Commands

```bash
# 1. Activate a license:
./nexkeyctl --activate --key=2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08

# 2. Check local license receipt status and resolved path:
./nexkeyctl --status

# 3. Synchronize license with the gateway:
./nexkeyctl --sync

# 4. Deactivate the license and release the seat:
./nexkeyctl --deactivate

# 5. Check for updates and vendor notices:
./nexkeyctl --updates --current-version=1.0.0 --base-url=https://sdk-sandbox.mcnexus.app --host-name=DaVinciResolve
```

---

## 5. Sandboxed Host Diagnostics

When running inside DaVinci Resolve or host environments installed from the Mac App Store or Windows MSIX AppContainer, application storage paths may be sandboxed and redirected. 

`nexkeyctl --status` explicitly prints the exact filesystem path resolved for receipts so you can verify that external activations align with the directory accessed by the plugin inside the host.
