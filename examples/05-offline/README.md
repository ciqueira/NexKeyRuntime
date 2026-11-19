# Offline Air-Gapped Licensing Example (05-offline)

This example demonstrates how to activate a workstation that cannot reach the internet (air-gapped suites, secure facilities, render farms).

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

*(Optional standalone build: `cmake -S examples/05-offline -B examples/05-offline/build && cmake --build examples/05-offline/build -j8`)*

---

## 2. Two-Step Offline Activation Workflow

```bash
# Step 1: Export machine request file (contains hardware binding and key prefix):
./build/bin/offline-example --export=my-workstation-request.json

# Step 2: Transfer my-workstation-request.json to the portal or vendor administrator.
#         Receive back the signed activation certificate (response.json).

# Step 3: Import signed certificate into the air-gapped machine:
./build/bin/offline-example --import=response.json
```

*(On Windows, use `.\build\bin\Release\offline-example.exe`.)*
