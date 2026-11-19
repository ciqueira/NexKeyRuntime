# Product Notices & Advisories Example (04-notices)

This example demonstrates how native plugins query the **NexKeyRuntime** SDK for vendor broadcast advisories, compatibility warnings, and host-targeted notices.

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

*(Optional standalone build: `cmake -S examples/04-notices -B examples/04-notices/build && cmake --build examples/04-notices/build -j8`)*

---

## 2. Testing Audience Rules & Dismissal

```bash
# 1. Query notices targeted to DaVinci Resolve:
./build/bin/notices-example --host-name=DaVinciResolve --host-version=20.0.1

# 2. Query notices targeted to another host (e.g. Adobe Premiere Pro):
./build/bin/notices-example --host-name=PremierePro --host-version=24.5

# 3. Test dismissing an active notice in-memory:
./build/bin/notices-example --host-name=DaVinciResolve --dismiss
```

*(On Windows, use `.\build\bin\Release\notices-example.exe`.)*
