// =============================================================================
// NexKeyRuntime SDK: 05 Offline Air-Gapped Licensing
// -----------------------------------------------------------------------------
// Demonstrates offline activation on machines without network connectivity:
//   1. Export activation request JSON identifying machine hardware binding.
//   2. Import signed activation certificate returned by administrator/portal.
// =============================================================================

#include <nexkeyruntime/nexkeyruntime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char **argv) {
    printf("=======================================================\n");
    printf("   NexKeyRuntime SDK: Offline Air-Gapped Licensing     \n");
    printf("   Environment: MCNexus Sandbox                        \n");
    printf("=======================================================\n\n");

    const char *export_path = "offline-request.json";
    const char *import_path = NULL;
    const char *key = "B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D";

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--export=", 9) == 0) {
            export_path = argv[i] + 9;
        } else if (strncmp(argv[i], "--import=", 9) == 0) {
            import_path = argv[i] + 9;
        } else if (strncmp(argv[i], "--key=", 6) == 0) {
            key = argv[i] + 6;
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

    NexKeyRuntimeResult pd_res = nexkeyruntime_license_set_product_data(handle, product_data);
    if (pd_res != NEXKEYRUNTIME_OK) {
        fprintf(stderr, "Error: set_product_data failed with code %d\n", (int)pd_res);
    }
    nexkeyruntime_license_set_tenant_id(handle, "mcnexus-sandbox");
    nexkeyruntime_license_set_variant(handle, "download:default");

    if (import_path != NULL) {
        printf("--> Importing offline activation certificate from: %s\n", import_path);
        char *cert_json = read_file_string(import_path);
        if (!cert_json) {
            fprintf(stderr, "Error: Could not read certificate file '%s'\n", import_path);
            nexkeyruntime_license_destroy(handle);
            free(product_data);
            return 1;
        }

        NexKeyRuntimeResult res = nexkeyruntime_license_publish_receipt(handle, cert_json);
        free(cert_json);

        if (res == NEXKEYRUNTIME_OK) {
            printf("\033[32m[OK] Certificate imported and verified successfully!\033[0m\n");
            NexKeyRuntimeRenderDecision decision = nexkeyruntime_license_render_decision(handle);
            printf("  Render Decision: %s\n", decision == NEXKEYRUNTIME_RENDER_ALLOW ? "ALLOW" : "DENY");
        } else {
            printf("\033[31m[FAILED] Certificate verification failed (code %d)\033[0m\n", (int)res);
        }
    } else {
        printf("--> Generating offline machine activation request...\n");
        if (key) {
            nexkeyruntime_license_set_license_key(handle, key);
        }

        NexKeyRuntimeResult res = nexkeyruntime_license_export_activation_request(handle, export_path);
        if (res == NEXKEYRUNTIME_OK) {
            printf("\033[32m[OK] Offline request written to: %s\033[0m\n", export_path);
            printf("     Transfer this file to an internet-connected machine or administrator portal to obtain a signed activation certificate.\n");
        } else {
            printf("\033[31m[FAILED] Could not export activation request (%d)\033[0m\n", (int)res);
        }
    }

    nexkeyruntime_license_destroy(handle);
    free(product_data);
    return 0;
}
