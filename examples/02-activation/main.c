// =============================================================================
// NexKeyRuntime SDK: 02 Profile B Autonomous Licensing
// -----------------------------------------------------------------------------
// Real Profile B implementation: the application or plugin activates and
// deactivates licenses autonomously against the Sandbox Gateway.
// =============================================================================

#include <nexkeyruntime/nexkeyruntime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TEST_KEY "B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D"
#define DEFAULT_TENANT "mcnexus-sandbox"
#define DEFAULT_VARIANT "default"

static char *read_file_string(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0) { fclose(f); return NULL; }
    char *buf = (char *)malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t read_bytes = fread(buf, 1, len, f);
    buf[read_bytes] = '\0';
    fclose(f);
    // Trim trailing whitespace and newlines
    while (read_bytes > 0 && (buf[read_bytes - 1] == '\n' || buf[read_bytes - 1] == '\r' || buf[read_bytes - 1] == ' ')) {
        buf[--read_bytes] = '\0';
    }
    return buf;
}

static char *resolve_product_data(void) {
    const char *env = getenv("NEXKEYRUNTIME_PRODUCT_FILE");
    if (env && env[0] != '\0') {
        char *data = read_file_string(env);
        if (data) return data;
    }
    const char *candidates[] = {
        "sandbox/mcnexus-sandbox.nexkeydat",
        "../sandbox/mcnexus-sandbox.nexkeydat",
        "../../sandbox/mcnexus-sandbox.nexkeydat",
        "../../../sandbox/mcnexus-sandbox.nexkeydat",
        "MCSDK/NexKeyRuntime/sandbox/mcnexus-sandbox.nexkeydat"
    };
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        char *data = read_file_string(candidates[i]);
        if (data) return data;
    }
    return NULL;
}

static const char *status_to_string(NexKeyRuntimeLicenseStatus s) {
    switch (s) {
        case NEXKEYRUNTIME_LICENSE_UNKNOWN: return "UNKNOWN";
        case NEXKEYRUNTIME_LICENSE_NOT_ACTIVATED: return "NOT_ACTIVATED";
        case NEXKEYRUNTIME_LICENSE_ACTIVATING: return "ACTIVATING";
        case NEXKEYRUNTIME_LICENSE_ACTIVE: return "ACTIVE";
        case NEXKEYRUNTIME_LICENSE_OFFLINE_GRACE: return "OFFLINE_GRACE";
        case NEXKEYRUNTIME_LICENSE_OFFLINE_GRACE_EXPIRED: return "OFFLINE_GRACE_EXPIRED";
        case NEXKEYRUNTIME_LICENSE_EXPIRED: return "EXPIRED";
        case NEXKEYRUNTIME_LICENSE_SUSPENDED: return "SUSPENDED";
        case NEXKEYRUNTIME_LICENSE_REVOKED: return "REVOKED";
        case NEXKEYRUNTIME_LICENSE_ACTIVATION_REMOVED: return "ACTIVATION_REMOVED";
        case NEXKEYRUNTIME_LICENSE_DEVICE_MISMATCH: return "DEVICE_MISMATCH";
        case NEXKEYRUNTIME_LICENSE_CERTIFICATE_INVALID: return "CERTIFICATE_INVALID";
        case NEXKEYRUNTIME_LICENSE_CLOCK_ROLLBACK: return "CLOCK_ROLLBACK";
        case NEXKEYRUNTIME_LICENSE_SERVICE_UNAVAILABLE: return "SERVICE_UNAVAILABLE";
        case NEXKEYRUNTIME_LICENSE_INTERNAL_ERROR: return "INTERNAL_ERROR";
        default: return "OTHER";
    }
}

static void on_license_status_changed(NexKeyRuntimeLicenseStatus status, void *user_data) {
    (void)user_data;
    printf("  [Callback] Background status changed: %s\n", status_to_string(status));
}

int main(int argc, char **argv) {
    printf("=======================================================\n");
    printf("   NexKeyRuntime SDK: Profile B Autonomous Activation  \n");
    printf("   Gateway: https://sdk-sandbox.mcnexus.app           \n");
    printf("=======================================================\n\n");

    const char *key = DEFAULT_TEST_KEY;
    int keep_active = 0;
    int deactivate_only = 0;

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--key=", 6) == 0) {
            key = argv[i] + 6;
        } else if (strcmp(argv[i], "--keep") == 0) {
            keep_active = 1;
        } else if (strcmp(argv[i], "--deactivate") == 0) {
            deactivate_only = 1;
        }
    }

    char *product_data = resolve_product_data();
    if (!product_data) {
        fprintf(stderr, "Error: Could not locate sandbox/mcnexus-sandbox.nexkeydat.\n");
        return 2;
    }

    NexKeyRuntimeLicenseHandle *handle = nexkeyruntime_license_create();
    if (!handle) {
        fprintf(stderr, "Error: Failed to create license handle.\n");
        free(product_data);
        return 1;
    }

    nexkeyruntime_license_set_product_data(handle, product_data);
    nexkeyruntime_license_set_tenant_id(handle, DEFAULT_TENANT);
    nexkeyruntime_license_set_variant(handle, DEFAULT_VARIANT);
    nexkeyruntime_license_set_metadata(handle, "appVersion", "1.0.0");
    nexkeyruntime_license_set_metadata(handle, "product", "ProfileBExample");
    nexkeyruntime_license_set_callback(handle, on_license_status_changed, NULL);

    // 1. Check existing local state
    nexkeyruntime_license_load_local(handle);

    if (deactivate_only) {
        printf("--> Deactivating local license...\n");
        NexKeyRuntimeResult deact_res = nexkeyruntime_license_deactivate(handle);
        if (deact_res == NEXKEYRUNTIME_OK) {
            printf("\033[32m[OK] Seat successfully released on gateway.\033[0m\n");
        } else {
            printf("\033[31m[FAILED] Deactivation returned error code %d\033[0m\n", (int)deact_res);
        }
        nexkeyruntime_license_destroy(handle);
        free(product_data);
        return deact_res == NEXKEYRUNTIME_OK ? 0 : 1;
    }

    // 2. Perform live network activation
    printf("--> Attempting activation with key: %s\n", key);
    nexkeyruntime_license_set_license_key(handle, key);

    NexKeyRuntimeResult act_res = nexkeyruntime_license_activate(handle);
    if (act_res == NEXKEYRUNTIME_OK) {
        printf("\033[32m[OK] License activation succeeded!\033[0m\n");
    } else {
        printf("\033[31m[DENIED] Activation failed with result code: %d\033[0m\n", (int)act_res);
        switch (act_res) {
            case NEXKEYRUNTIME_E_LICENSE_KEY:
                printf("  Reason: Invalid license key.\n"); break;
            case NEXKEYRUNTIME_E_ACTIVATION_LIMIT:
                printf("  Reason: Maximum device seats reached.\n"); break;
            case NEXKEYRUNTIME_E_LICENSE_EXPIRED:
                printf("  Reason: License has expired.\n"); break;
            case NEXKEYRUNTIME_E_LICENSE_SUSPENDED:
                printf("  Reason: License is suspended/revoked.\n"); break;
            case NEXKEYRUNTIME_NETWORK_ERROR:
                printf("  Reason: Network connectivity error.\n"); break;
            default:
                printf("  Reason: Other error (%d)\n", (int)act_res); break;
        }
        nexkeyruntime_license_destroy(handle);
        free(product_data);
        return 1;
    }

    // 3. Inspect verified snapshot and render decision
    NexKeyRuntimeLicenseSnapshot snapshot;
    snapshot.struct_size = sizeof(snapshot);
    if (nexkeyruntime_license_get_snapshot(handle, &snapshot) == NEXKEYRUNTIME_OK) {
        printf("\nVerified Snapshot:\n");
        printf("  Status          : %s\n", status_to_string(snapshot.status));
        printf("  Edition         : %s\n", snapshot.edition[0] ? snapshot.edition : "default");
        printf("  Days Remaining  : %d\n", snapshot.days_remaining);
        if (snapshot.max_activations > 0) {
            printf("  Seats           : %d / %d\n", snapshot.activations_used, snapshot.max_activations);
        }
    }

    NexKeyRuntimeRenderDecision decision = nexkeyruntime_license_render_decision(handle);
    printf("  Render Decision : %s\n\n", decision == NEXKEYRUNTIME_RENDER_ALLOW ? "\033[32mALLOW\033[0m" : "\033[31mDENY\033[0m");

    // 4. Optionally test clean deactivation
    if (!keep_active) {
        printf("--> Testing clean deactivation (releasing seat)...\n");
        NexKeyRuntimeResult deact_res = nexkeyruntime_license_deactivate(handle);
        if (deact_res == NEXKEYRUNTIME_OK) {
            printf("\033[32m[OK] Seat released cleanly.\033[0m\n");
            printf("     (Pass '--keep' to retain activation for host testing).\n");
        } else {
            printf("\033[31m[FAILED] Deactivation error (%d)\033[0m\n", (int)deact_res);
        }
    } else {
        printf("--> Activation retained on machine (--keep specified).\n");
    }

    nexkeyruntime_license_destroy(handle);
    free(product_data);
    return 0;
}
