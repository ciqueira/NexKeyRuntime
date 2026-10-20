# nexkeyctl: NexKeyRuntime Command-Line Control Tool

`nexkeyctl` is the official native CLI tool for activating, deactivating, inspecting license states, checking update releases, and displaying product notices backed by the **NexKeyRuntime** SDK.

---

## 1. Building `nexkeyctl`

```bash
# From the repository root or tools directory:
cmake -S tools/nexkeyctl -B tools/nexkeyctl/build
cmake --build tools/nexkeyctl/build -j8
```

*Binary executable location:*
- macOS / Linux: `tools/nexkeyctl/build/nexkeyctl`
- Windows: `tools/nexkeyctl/build/Release/nexkeyctl.exe`

---

## 2. Usage & Commands

```bash
# Display help and usage
./nexkeyctl --help

# 1. Activate a test license:
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

## 3. Sandboxed Host Diagnostics (P11)

When running inside DaVinci Resolve or host environments installed from the Mac App Store or Windows MSIX AppContainer, application storage paths may be sandboxed and redirected. 

`nexkeyctl --status` explicitly prints the exact filesystem path resolved for receipts so you can verify that external activations align with the directory accessed by the plugin inside the host.

