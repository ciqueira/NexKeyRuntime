# Release Updates Discovery Example (03-updates)

This example demonstrates how native plugins and desktop products query the **NexKeyRuntime** SDK for newer software releases across channels (`stable`, `beta`, `nightly`).

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

*(Optional standalone build: `cmake -S examples/03-updates -B examples/03-updates/build && cmake --build examples/03-updates/build -j8`)*

---

## 2. Running Live Release Checks

```bash
# 1. Run check simulating version 1.0.0 on stable channel:
./build/bin/updates-example --current-version=1.0.0

# 2. Simulate checking against a specific channel (e.g. beta):
./build/bin/updates-example --current-version=1.0.0 --channel=beta
```

*(On Windows, use `.\build\bin\Release\updates-example.exe`.)*
