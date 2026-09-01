/* Profile B: your product activates the license itself, with no host
 * application in the loop.
 *
 * Profile A (examples/minimal) is the common case for plugins — MCNexus
 * activates and your plugin only verifies. Use this one when your product
 * owns the whole relationship: it collects the key, activates, and gives the
 * seat back.
 *
 * Not buildable standalone in this repository — see examples/cmake-consumer.
 */

#include <nexkeyruntime/nexkeyruntime.h>

#include <stdio.h>
#include <stddef.h>

static const char *NEXKEYRUNTIME_PRODUCT_DATA =
    "eyJmb3JtYXQiOiJuZXhrZXlydW50aW1lLXByb2R1Y3RkYXRhLXYxIiwi...";

static NexKeyRuntimeLicenseHandle *g_license = NULL;

/* Runs on one of the SDK's own threads, not yours. Record and return: do not
 * block, do not call back into the handle, and do not touch your UI from
 * here — marshal to your own thread first. */
static void on_license_changed(NexKeyRuntimeLicenseStatus status, void *user_data) {
    (void)user_data;
    (void)status;
    /* e.g. set a flag your UI thread reads */
}

static void example_startup(void) {
    g_license = nexkeyruntime_license_create();

    nexkeyruntime_license_set_product_data(g_license, NEXKEYRUNTIME_PRODUCT_DATA);
    nexkeyruntime_license_set_tenant_id(g_license, "your-tenant-id");
    nexkeyruntime_license_set_variant(g_license, "download:your-entitlement");

    /* Reported to the backend so support can tell which build is deployed.
     * Nothing in the licensing decision reads it. */
    nexkeyruntime_license_set_metadata(g_license, "appVersion", "1.4.2");

    nexkeyruntime_license_set_callback(g_license, on_license_changed, NULL);

    /* Reads whatever receipt is already on disk. No network. */
    nexkeyruntime_license_load_local(g_license);
}

/* Blocks on the network. Call from a worker or UI thread with a spinner —
 * never from a render callback. */
static int example_activate(const char *user_entered_key) {
    nexkeyruntime_license_set_license_key(g_license, user_entered_key);

    const NexKeyRuntimeResult result = nexkeyruntime_license_activate(g_license);
    switch (result) {
    case NEXKEYRUNTIME_OK:
        /* The receipt is on disk and render_decision() now answers ALLOW. */
        return 1;

    /* Worth distinguishing in your UI: each needs a different sentence. */
    case NEXKEYRUNTIME_E_LICENSE_KEY:        /* not a key we issued          */
    case NEXKEYRUNTIME_E_ACTIVATION_LIMIT:   /* every seat is in use         */
    case NEXKEYRUNTIME_E_LICENSE_EXPIRED:
    case NEXKEYRUNTIME_E_LICENSE_SUSPENDED:
    case NEXKEYRUNTIME_E_LICENSE_REVOKED:
    case NEXKEYRUNTIME_E_WRONG_TENANT:       /* a key for another product    */
        fprintf(stderr, "activation refused (%d)\n", (int)result);
        return 0;

    case NEXKEYRUNTIME_NETWORK_ERROR:
        /* Nothing was consumed. Offer a retry — and, if the machine has no
         * network at all, examples/offline. */
        fprintf(stderr, "could not reach the backend\n");
        return 0;

    default:
        /* Treat unknown values as a generic failure, never as success: the
         * result enum is append-only and a future version may add codes. */
        fprintf(stderr, "activation failed (%d)\n", (int)result);
        return 0;
    }
}

static void example_show_status(void) {
    NexKeyRuntimeLicenseSnapshot snapshot;
    snapshot.struct_size = sizeof(snapshot);
    if (nexkeyruntime_license_get_snapshot(g_license, &snapshot) != NEXKEYRUNTIME_OK) {
        return;
    }

    /* Use `status` for the message. Never infer it from render_decision(),
     * which answers ALLOW/DENY and not why. */
    printf("status: %d  edition: %s\n", (int)snapshot.status, snapshot.edition);

    if (snapshot.max_activations > 0) {
        printf("seats: %d/%d\n", snapshot.activations_used, snapshot.max_activations);
    } else {
        /* Seat counts come from the server, not from the receipt, so a purely
         * local read genuinely does not know them. Printing 0/0 would read as
         * "no seats in use", which is wrong. */
        printf("seats: unknown until the next sync\n");
    }
}

/* The hot path. One atomic read: no I/O, no lock, no allocation. */
static int example_render(void) {
    return nexkeyruntime_license_render_decision(g_license) == NEXKEYRUNTIME_RENDER_ALLOW;
}

static void example_deactivate(void) {
    /* Releases the seat server-side and removes this machine's proof. */
    if (nexkeyruntime_license_deactivate(g_license) == NEXKEYRUNTIME_OK) {
        printf("seat released\n");
    }
}

static void example_shutdown(void) {
    /* Stops and joins the SDK's background work before returning, so a host
     * may unload your bundle immediately afterwards. */
    nexkeyruntime_license_destroy(g_license);
    g_license = NULL;
}

int main(void) {
    example_startup();
    if (example_activate("XXXX-YYYY-ZZZZ-WWWW-VVVV")) {
        example_show_status();
        printf("render: %s\n", example_render() ? "ALLOW" : "DENY");
        example_deactivate();
    }
    example_shutdown();
    return 0;
}
