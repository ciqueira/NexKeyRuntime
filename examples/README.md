# NexKeyRuntime SDK: Minimal Feature Examples

This directory provides clean, minimal, single-purpose code examples for each isolated feature of the **NexKeyRuntime** SDK.

If you are looking for the **OpenFX reference plugin** that runs inside **DaVinci Resolve** with UI controls, watermarking, update checks, notices, and optional real-time MIDI hardware control (NexControl), see [**`sandbox/ofx-plugin/`**](../sandbox/ofx-plugin/).

All commands below assume your working directory is the **repository root** (`NexKeyRuntime/`).

---

## 1. Directory Matrix

| Directory | Feature | Description |
| :--- | :--- | :--- |
| [**01-minimal**](01-minimal) | Profile A (Offline Render) | Fast check: loads local receipt, reports edition, evaluates atomic `render_decision()` in `< 1 ns`. |
| [**02-activation**](02-activation) | Profile B (Autonomous Licensing) | Collects key, activates via HTTPS, checks seats, and deactivates to release seat. |
| [**03-updates**](03-updates) | Release Discovery | Asynchronous semver check against Sandbox manifest, channel filtering (`stable`/`beta`). |
| [**04-notices**](04-notices) | Product Advisories | Host-targeted compatibility notices, user dismissal, and reminders. |
| [**05-offline**](05-offline) | Air-Gapped Licensing | Hardware fingerprint request export and offline signed certificate import. |
| [**06-nexcontrol-ofx**](06-nexcontrol-ofx) | Hardware MIDI Control | OpenFX plugin integration with the NexControl real-time MIDI hardware subsystem (requires `libnexcontrol`). |

---

## 2. Building CLI Examples (01–05) from the Repository Root

Place the downloaded release static library in `<repo>/lib/` (`lib/libnexkeyruntime.a` on macOS, `lib/nexkeyruntime.lib` on Windows), then run:

**macOS:**
```bash
cmake -S . -B build
cmake --build build -j8
```
*Output binaries:* `build/bin/`

**Windows:**
```cmd
cmake -S . -B build -A x64
cmake --build build --config Release
```
*(Use `-A ARM64` instead of `-A x64` if you downloaded `windows-arm64.zip`; `-A` must match your downloaded release archive architecture.)*
*Output binaries:* `build\bin\Release\`

---

## 3. Building the NexControl OpenFX Example (06)

Because `06-nexcontrol-ofx` targets OpenFX and links `libnexcontrol.a` / `nexcontrol.lib`, place `libnexcontrol` in `<repo>/lib/` and enable OFX targets from the repository root:

**macOS:**
```bash
cmake -S . -B build -DNEXKEY_BUILD_OFX=ON
cmake --build build -j8
```
*Artifact output:* `build/plugins/NexControlSamplePlugin.ofx.bundle`

**Windows:**
```cmd
cmake -S . -B build -A x64 -DNEXKEY_BUILD_OFX=ON
cmake --build build --config Release
```
*Artifact output:* `build\plugins\Release\NexControlSamplePlugin.ofx.bundle`

Install the resulting `.ofx.bundle` to your host plugin path (`/Library/OFX/Plugins/` on macOS or `C:\Program Files\Common Files\OFX\Plugins\` on Windows) to test with DaVinci Resolve.

---

## 4. Running Each Minimal Example (from Repository Root)

**macOS:**
```bash
# 1. Profile A: Verify local license decision:
./build/bin/license-validator

# 2. Profile B: Test live activation on the Sandbox Gateway:
./build/bin/profile-b-example --key=B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D

# 3. Check for release updates:
./build/bin/updates-example --current-version=1.0.0

# 4. Check for host notices:
./build/bin/notices-example --host-name=DaVinciResolve

# 5. Export offline machine activation request:
./build/bin/offline-example --export=machine-request.json
```

**Windows:** Replace `./build/bin/<name>` with `.\build\bin\Release\<name>.exe`.
