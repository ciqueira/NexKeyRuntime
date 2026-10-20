// =============================================================================
// NexKeyRuntime SDK: 03 Release Updates Discovery
// -----------------------------------------------------------------------------
// Demonstrates how to discover releases, compare semver, and inspect channels
// using NexKeyRuntimeHandle against the Sandbox Gateway.
// =============================================================================

#include <nexkeyruntime/nexkeyruntime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#define sleep_ms(x) Sleep(x)
#else
#include <unistd.h>
#define sleep_ms(x) usleep((x) * 1000)
#endif

static int example_open_url(void *user_data, const char *url) {
    (void)user_data;
    printf("  [Action] Launching URL: %s\n", url);
    return 1;
}

static int example_can_open_url(void *user_data, const char *url) {
    (void)user_data;
    return (url != NULL && url[0] != '\0');
}

int main(int argc, char **argv) {
    printf("=======================================================\n");
    printf("   NexKeyRuntime SDK: Release Updates Discovery        \n");
    printf("   Environment: MCNexus Sandbox                        \n");
    printf("=======================================================\n\n");

    const char *current_version = "1.0.0";
    const char *channel = "stable";
    const char *base_url = "https://sdk-sandbox.mcnexus.app";

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--current-version=", 18) == 0) {
            current_version = argv[i] + 18;
        } else if (strncmp(argv[i], "--channel=", 10) == 0) {
            channel = argv[i] + 10;
        } else if (strncmp(argv[i], "--base-url=", 11) == 0) {
            base_url = argv[i] + 11;
        }
    }

    NexKeyRuntimeConfig config;
    nexkeyruntime_config_init(&config);

    config.tenant_id = "mcnexus-sandbox";
    config.artifact_id = "default";
    config.base_url = base_url;
    config.current_version = current_version;
    config.channel = channel;
    config.locale = "en-US";

#if defined(__APPLE__)
    config.platform = "macos";
#elif defined(_WIN32)
    config.platform = "windows";
#else
    config.platform = "linux";
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
    config.architecture = "arm64";
#else
    config.architecture = "x86_64";
#endif

    config.open_url = example_open_url;
    config.can_open_url = example_can_open_url;

    printf("Configuration:\n");
    printf("  Tenant       : %s\n", config.tenant_id);
    printf("  Artifact     : %s\n", config.artifact_id);
    printf("  Current Ver  : %s\n", config.current_version);
    printf("  Platform     : %s (%s)\n", config.platform, config.architecture);
    printf("  Channel      : %s\n", config.channel);
    printf("  Endpoint     : %s\n\n", config.base_url);

    NexKeyRuntimeHandle *handle = nexkeyruntime_create(&config);
    if (!handle) {
        fprintf(stderr, "Error: Invalid configuration provided to nexkeyruntime_create.\n");
        return 1;
    }

    printf("--> Requesting background release check from gateway...\n");
    nexkeyruntime_request_check(handle, 1 /* force = 1 bypasses schedule */);

    // Wait up to 3 seconds for async HTTPS check
    NexKeyRuntimeUpdateSnapshot snapshot;
    snapshot.struct_size = sizeof(snapshot);
    int wait_cycles = 0;
    while (wait_cycles < 30) {
        sleep_ms(100);
        wait_cycles++;
        if (nexkeyruntime_get_snapshot(handle, &snapshot) == NEXKEYRUNTIME_OK) {
            if (snapshot.status != NEXKEYRUNTIME_UPDATE_CHECKING) {
                break;
            }
        }
    }

    printf("\nCheck Result:\n");
    switch (snapshot.status) {
        case NEXKEYRUNTIME_UPDATE_UP_TO_DATE:
            printf("  Status: \033[32mUP_TO_DATE\033[0m: You are running the newest release.\n");
            break;
        case NEXKEYRUNTIME_UPDATE_AVAILABLE:
            printf("  Status: \033[36mUPDATE_AVAILABLE\033[0m\n");
            printf("  Newer Version Available: \033[32m%s\033[0m\n", snapshot.latest_version);
            printf("  Notes URL               : %s\n", snapshot.notes_url[0] ? snapshot.notes_url : "(none)");
            printf("  Deep Link               : %s\n", snapshot.deep_link[0] ? snapshot.deep_link : "(none)");
            printf("  Fallback URL            : %s\n", snapshot.fallback_url[0] ? snapshot.fallback_url : "(none)");
            printf("\n--> Triggering update action launcher:\n");
            nexkeyruntime_open_update_action(handle);
            break;
        case NEXKEYRUNTIME_UPDATE_CHECKING:
            printf("  Status: Check still in progress (network latency).\n");
            break;
        default:
            printf("  Status: Unable to reach gateway (%d) or no release published for this channel.\n", (int)snapshot.status);
            break;
    }

    nexkeyruntime_destroy(handle);
    return 0;
}
