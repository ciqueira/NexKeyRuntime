# Profile B Autonomous Licensing Example

This example demonstrates **Profile B** integration of the **NexKeyRuntime** SDK.

In Profile B, the native product (application, service, or self-activating plugin) collects a license key directly from the user, connects over HTTPS to the licensing gateway, saves the verified Ed25519 activation certificate locally, and releases seats upon deactivation.

---

## 1. Building the Example

```bash
cmake -S examples/02-profile-b-app -B examples/02-profile-b-app/build
cmake --build examples/02-profile-b-app/build
```

---

## 2. Running Live Activations against the Sandbox

```bash
# 1. Test standard activation & deactivation cycle (default Full license):
./examples/02-profile-b-app/build/profile-b-example

# 2. Activate and keep active on machine (to test with OFX plugins or host):
./examples/02-profile-b-app/build/profile-b-example --key=B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D --keep

# 3. Test negative case: Suspended key (returns E_LICENSE_SUSPENDED):
./examples/02-profile-b-app/build/profile-b-example --key=AF65F0-A84D1C-51F3B6-2E3139-4C5931-534E28FA

# 4. Deactivate currently stored license:
./examples/02-profile-b-app/build/profile-b-example --deactivate
```

