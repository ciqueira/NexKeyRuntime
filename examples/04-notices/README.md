# Product Notices & Advisories Example

This example demonstrates how native plugins query the **NexKeyRuntime** SDK for vendor broadcast advisories, compatibility warnings, and host-targeted notices.

---

## 1. Building the Example

```bash
cmake -S examples/04-notices -B examples/04-notices/build
cmake --build examples/04-notices/build
```

---

## 2. Testing Audience Rules & Dismissal

```bash
# 1. Query notices targeted to DaVinci Resolve:
./examples/04-notices/build/notices-example --host-name=DaVinciResolve --host-version=20.0.1

# 2. Query notices targeted to another host (e.g. Adobe Premiere Pro):
./examples/04-notices/build/notices-example --host-name=PremierePro --host-version=24.5

# 3. Test dismissing an active notice in-memory:
./examples/04-notices/build/notices-example --host-name=DaVinciResolve --dismiss
```

