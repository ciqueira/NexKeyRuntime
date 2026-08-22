# Integration guide (Profile A)

This is the supported integration path today: the host application
(MCNexus) activates and syncs the license; your plugin only verifies.
Profile B (the product activating on its own) is implemented as well and is
covered further down; third-party use of the compiled binaries is still gated
on the draft binary license — see the [README](../README.md).

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

## Profile B — activating from your own plugin

Profile A above covers the common case: MCNexus activates, your plugin only
verifies. If your plugin activates on its own instead, the additional calls
are `nexkeyruntime_license_set_license_key`, `_activate`, `_deactivate`,
`_request_sync` and `_publish_receipt`.

```c
nexkeyruntime_license_set_license_key(g_license, user_entered_key);
nexkeyruntime_license_set_metadata(g_license, "appVersion", "1.4.2");
if (nexkeyruntime_license_activate(g_license) == NEXKEYRUNTIME_OK) {
    /* the receipt is on disk and render_decision() now answers ALLOW */
}
```

`_activate`, `_deactivate` and `_request_sync` block on the network for up to
their timeout. Call them from a UI or worker thread, never from a render
callback.

Once a license is active the SDK keeps it fresh on its own, on a background
thread, and you do not have to schedule anything. Subscribe if you want to
know when something changes:

```c
static void on_license(NexKeyRuntimeLicenseStatus status, void *user_data) {
    /* Runs on the SDK's poller thread. Do not block, and do not call back
       into the handle from here — record the status and return. */
}
nexkeyruntime_license_set_callback(g_license, on_license, NULL);
```

Two behaviours worth knowing, because they are deliberate:

- **A headless process does not sync.** A render node opening the same project
  on 200 machines will not produce 200 requests; each one decides offline from
  the receipt it already has. Override with `_set_headless` in either
  direction if the autodetection is wrong for your host.
- **Many instances share one background thread.** Fifty nodes of the same
  effect in one project produce one poller, not fifty. You do not need to
  deduplicate handles yourself.

`nexkeyruntime_license_destroy` stops and joins that thread before it returns,
so it is safe for your host to unload the bundle immediately afterwards.
