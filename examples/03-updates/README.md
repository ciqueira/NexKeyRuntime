# Release Updates Discovery Example

This example demonstrates how native plugins and desktop products query the **NexKeyRuntime** SDK for newer software releases across channels (`stable`, `beta`, `nightly`).

---

## 1. Building the Example

```bash
cmake -S examples/03-updates -B examples/03-updates/build
cmake --build examples/03-updates/build
```

---

## 2. Running Live Release Checks

```bash
# 1. Run check simulating version 1.0.0 on stable channel:
./examples/03-updates/build/updates-example --current-version=1.0.0

# 2. Simulate checking against a specific channel (e.g. beta):
./examples/03-updates/build/updates-example --current-version=1.0.0 --channel=beta
```

