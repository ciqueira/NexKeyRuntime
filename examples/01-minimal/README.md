# Profile A Offline License Validator Example (01-minimal)

This example demonstrates **Profile A** local receipt verification using the C++ wrapper [`include/MCLicense.h`](../../include/MCLicense.h).

In Profile A, the application or plugin never makes network requests on the render path: it loads the locally stored Ed25519 receipt once and evaluates `allowed()` (`nexkeyruntime_license_render_decision`) in `< 1 ns`.

All commands below assume your working directory is the **repository root** (`NexKeyRuntime/`).

---

## 1. Building from the Repository Root

**macOS:**
```bash
cmake -S . -B build
cmake --build build -j8
```

**Windows:**
```cmd
cmake -S . -B build -A x64
cmake --build build --config Release
```
*(Use `-A ARM64` instead of `-A x64` if you downloaded `windows-arm64.zip`; `-A` must match your downloaded release archive architecture.)*

*(Optional standalone build: `cmake -S examples/01-minimal -B examples/01-minimal/build && cmake --build examples/01-minimal/build -j8`)*

---

## 2. Running the Validator

```bash
# 1. Inspect current local license state:
./build/bin/license-validator

# 2. If no receipt is active yet, activate a Sandbox Demo key first:
./build/bin/nexkeyctl --activate \
  --key=2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08 \
  --tenant=mcnexus-sandbox \
  --product-file=sandbox/mcnexus-sandbox.nexkeydat

# 3. Re-run the validator to confirm ALLOW + DEMO edition:
./build/bin/license-validator
```

*(On Windows, use `.\build\bin\Release\license-validator.exe` and `.\build\bin\Release\nexkeyctl.exe`.)*
