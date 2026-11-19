# Profile B Autonomous Licensing Example (02-activation)

This example demonstrates **Profile B** integration of the **NexKeyRuntime** SDK.

In Profile B, the native product (application, service, or self-activating plugin) collects a license key directly from the user, connects over HTTPS to the licensing gateway, saves the verified Ed25519 activation certificate locally, and releases seats upon deactivation.

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

*(Optional standalone build: `cmake -S examples/02-activation -B examples/02-activation/build && cmake --build examples/02-activation/build -j8`)*

---

## 2. Running Live Activations against the Sandbox

```bash
# 1. Test standard activation & deactivation cycle (default Full license):
./build/bin/profile-b-example

# 2. Activate and keep active on machine (to test with OFX plugins or host):
./build/bin/profile-b-example --key=B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D --keep

# 3. Test negative case: Suspended key (returns E_LICENSE_SUSPENDED):
./build/bin/profile-b-example --key=AF65F0-A84D1C-51F3B6-2E3139-4C5931-534E28FA

# 4. Deactivate currently stored license:
./build/bin/profile-b-example --deactivate
```

*(On Windows, use `.\build\bin\Release\profile-b-example.exe`.)*
