# Integration guide (Perfil A)

This is the supported integration path today: the host application
(MCNexus) activates and syncs the license; your plugin only verifies.
Perfil B (the plugin activating on its own) is declared in the header but
not yet operational — see the [README](../README.md).

## Lifecycle

### 1. Load (once per process)

```c
g_license = nexkeyruntime_license_create();
nexkeyruntime_license_set_product_data(g_license, NEXKEYRUNTIME_PRODUCT_DATA);
nexkeyruntime_license_set_tenant_id(g_license, "your-tenant-id");
nexkeyruntime_license_set_variant(g_license, "download:your-entitlement");
nexkeyruntime_license_load_local(g_license);
```

`ProductData` carries a base URL and a public-key keyring — never a secret.
It is signed by the tenant's private key and self-verified against its own
keyring on load. `tenant_id` and `variant` are separate runtime calls, not
fields decoded from the blob: the same ProductData blob can be reused across
every plugin variant a tenant ships, with the exact entitlement being
checked declared by each plugin at load time.

`load_local` reads whatever activation receipt MCNexus already wrote to
disk, verifies its Ed25519 signature against the ProductData keyring, checks
machine binding and every time window, and produces an ALLOW/DENY decision
that is cached in memory as a single atomic value. Nothing here touches the
network.

### 2. UI sync (any thread with time to spare)

```c
NexKeyRuntimeLicenseSnapshot snapshot;
if (nexkeyruntime_license_get_snapshot(g_license, &snapshot) == NEXKEYRUNTIME_OK) {
    // snapshot.status is one of the 15 NexKeyRuntimeLicenseStatus values
    // (see the header): UNKNOWN, NOT_ACTIVATED, ACTIVATING, ACTIVE,
    // OFFLINE_GRACE, OFFLINE_GRACE_EXPIRED, EXPIRED, SUSPENDED, REVOKED,
    // ACTIVATION_REMOVED, DEVICE_MISMATCH, CERTIFICATE_INVALID,
    // CLOCK_ROLLBACK, SERVICE_UNAVAILABLE, INTERNAL_ERROR. Local
    // verification (load_local) only ever produces a subset of these —
    // see the header comment above the enum. Use status to render an
    // accurate message — never infer it from render_decision, which only
    // answers ALLOW/DENY, not why.
}
```

### 3. Render (hot path — the one rule that matters)

```c
if (nexkeyruntime_license_render_decision(g_license) == NEXKEYRUNTIME_RENDER_ALLOW) {
    processProtectedEffect();
} else {
    copyInputToOutput(); // deterministic bypass or watermark
}
```

`render_decision` is a single atomic read with zero I/O. Never call
`get_snapshot`, `load_local`, or any network/file-touching function from a
render callback — those belong in load or a UI-thread sync, never here.

### 4. Unload

```c
nexkeyruntime_license_destroy(g_license);
g_license = NULL;
```

## What this repository does not cover yet

`nexkeyruntime_license_set_license_key`, `_activate`, `_deactivate`,
`_request_sync`, and `_publish_receipt` are declared in the header for
Perfil B (autonomous, host-free activation) but the backend sync network
they depend on has not shipped. Do not build against them yet — this
document will be updated once Perfil B is operational.
