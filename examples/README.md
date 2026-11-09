# NexKeyRuntime SDK: Minimal Feature Examples

This directory provides clean, minimal, single-purpose code examples for each isolated feature of the **NexKeyRuntime** SDK.

If you are looking for the **OpenFX reference plugin** that runs inside **DaVinci Resolve** with UI controls, watermarking, update checks, notices, and real-time MIDI hardware control (NexControl), see [**`sandbox/ofx-plugin/`**](../sandbox/ofx-plugin/).

---

## 1. Directory Matrix

| Directory | Feature | Description |
| :--- | :--- | :--- |
| [**01-minimal**](01-minimal) | Profile A (Offline Render) | Fast check: loads local receipt, reports edition, evaluates atomic `render_decision()` in `< 1 ns`. |
| [**02-activation**](02-activation) | Profile B (Autonomous Licensing) | Collects key, activates via HTTPS, checks seats, and deactivates to release seat. |
| [**03-updates**](03-updates) | Release Discovery | Asynchronous semver check against Sandbox manifest, channel filtering (`stable`/`beta`). |
| [**04-notices**](04-notices) | Product Advisories | Host-targeted compatibility notices, user dismissal, and reminders. |
| [**05-offline**](05-offline) | Air-Gapped Licensing | Hardware fingerprint request export and offline signed certificate import. |
| [**06-nexcontrol-ofx**](06-nexcontrol-ofx) | Hardware MIDI Control | OpenFX plugin integration with the NexControl real-time MIDI hardware subsystem. |

---

## 2. Building All Minimal CLI Examples (01-05)

```bash
cmake -S examples -B examples/build
cmake --build examples/build -j8
```

---

## 3. Building the NexControl OpenFX Example (06)

Because `06-nexcontrol-ofx` targets the OpenFX standard, it compiles as an `.ofx.bundle` shared module:

```bash
cmake -S examples/06-nexcontrol-ofx -B examples/06-nexcontrol-ofx/build
cmake --build examples/06-nexcontrol-ofx/build -j8
```

After building, install the resulting bundle to your host plugin path (e.g., `/Library/OFX/Plugins` on macOS or `C:\Program Files\Common Files\OFX\Plugins` on Windows) to test with DaVinci Resolve.

---

## 4. Running Each Minimal Example

```bash
# 1. Profile A: Verify local license decision:
./examples/build/01-minimal/license-validator

# 2. Profile B: Test live activation on the Sandbox Gateway:
./examples/build/02-activation/profile-b-example --key=B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D

# 3. Check for release updates:
./examples/build/03-updates/updates-example --current-version=1.0.0

# 4. Check for host notices:
./examples/build/04-notices/notices-example --host-name=DaVinciResolve

# 5. Export offline machine activation request:
./examples/build/05-offline/offline-example --export=machine-request.json
```
