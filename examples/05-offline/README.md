# Offline Air-Gapped Licensing Example

This example demonstrates how to activate a workstation that cannot reach the internet (air-gapped suites, secure facilities, render farms).

---

## 1. Building the Example

```bash
cmake -S examples/05-offline -B examples/05-offline/build
cmake --build examples/05-offline/build
```

---

## 2. Two-Step Offline Activation Workflow

```bash
# Step 1: Export machine request file (contains hardware binding and key prefix):
./examples/05-offline/build/offline-example --export=my-workstation-request.json

# Step 2: Transfer my-workstation-request.json to the portal or vendor administrator.
#         Receive back the signed activation certificate (response.json).

# Step 3: Import signed certificate into the air-gapped machine:
./examples/05-offline/build/offline-example --import=response.json
```

