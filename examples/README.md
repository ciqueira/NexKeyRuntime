# NexKeyRuntime SDK: Minimal Feature Examples

This directory provides clean, minimal, single-purpose code examples for each isolated feature of the **NexKeyRuntime** SDK.

If you are looking for the **complete, interactive OpenFX reference plugin** that runs inside **DaVinci Resolve** with UI controls, watermarking, update checks, and notices, see [**`sandbox/ofx-plugin/`**](../sandbox/ofx-plugin/).

---

## 1. Directory Matrix

| Directory | Feature | Description |
| :--- | :--- | :--- |
| [**01-minimal**](01-minimal) | Profile A (Offline Render) | Fast check: loads local receipt, reports edition, evaluates atomic `render_decision()` in `< 1 ns`. |
| [**02-activation**](02-activation) | Profile B (Autonomous Licensing) | Collects key, activates via HTTPS, checks seats, and deactivates to release seat. |
| [**03-updates**](03-updates) | Release Discovery | Asynchronous semver check against Sandbox manifest, channel filtering (`stable`/`beta`). |
| [**04-notices**](04-notices) | Product Advisories | Host-targeted compatibility notices, user dismissal, and reminders. |
| [**05-offline**](05-offline) | Air-Gapped Licensing | Hardware fingerprint request export and offline signed certificate import. |

---

## 2. Building All Minimal Examples

```bash
cmake -S examples -B examples/build
cmake --build examples/build -j8
```

---

## 3. Running Each Minimal Example

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
