// =============================================================================
// NexKeyRuntime SDK: 04 Product Notices & Vendor Advisories
// -----------------------------------------------------------------------------
// Demonstrates how to fetch vendor broadcast notices, filter by host audience,
// and handle dismiss and remind-later user interactions.
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
    printf("  [Action] Launching notice URL: %s\n", url);
    return 1;
}

static int example_can_open_url(void *user_data, const char *url) {
    (void)user_data;
    return (url != NULL && url[0] != '\0');
}

static const char *severity_name(NexKeyRuntimeNoticeSeverity s) {
    switch (s) {
        case NEXKEYRUNTIME_NOTICE_INFO: return "INFO";
        case NEXKEYRUNTIME_NOTICE_RECOMMENDED: return "RECOMMENDED";
        case NEXKEYRUNTIME_NOTICE_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

int main(int argc, char **argv) {
    printf("=======================================================\n");
    printf("   NexKeyRuntime SDK: Product Notices & Advisories     \n");
    printf("   Environment: MCNexus Sandbox                        \n");
    printf("=======================================================\n\n");

    const char *host_name = "DaVinciResolve";
    const char *host_version = "20.0.1";
    int do_dismiss = 0;
    int do_remind = 0;

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--host-name=", 12) == 0) {
            host_name = argv[i] + 12;
        } else if (strncmp(argv[i], "--host-version=", 15) == 0) {
            host_version = argv[i] + 15;
        } else if (strcmp(argv[i], "--dismiss") == 0) {
            do_dismiss = 1;
        } else if (strcmp(argv[i], "--remind-later") == 0) {
            do_remind = 1;
        }
    }

    NexKeyRuntimeConfig config;
    nexkeyruntime_config_init(&config);

    config.tenant_id = "mcnexus-sandbox";
    config.artifact_id = "default";
    config.base_url = "https://sdk-sandbox.mcnexus.app";
    config.current_version = "1.0.0";
    config.channel = "stable";
    config.locale = "en-US";

    // Host parameters for targeted audience filtering
    config.host_name = host_name;
    config.host_version = host_version;

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

    printf("Simulated Host Environment:\n");
    printf("  Host Application : %s\n", config.host_name);
    printf("  Host Version     : %s\n", config.host_version);
    printf("  Endpoint         : %s\n\n", config.base_url);

    NexKeyRuntimeHandle *handle = nexkeyruntime_create(&config);
    if (!handle) {
        fprintf(stderr, "Error: Invalid configuration provided to nexkeyruntime_create.\n");
        return 1;
    }

    printf("--> Querying active notices...\n");
    nexkeyruntime_request_check(handle, 1 /* force */);

    // Wait briefly for notice delivery
    sleep_ms(300);

    NexKeyRuntimeNotice notice;
    notice.struct_size = sizeof(notice);
    NexKeyRuntimeResult res = nexkeyruntime_get_active_notice(handle, &notice);

    if (res == NEXKEYRUNTIME_OK && notice.available) {
        printf("\n\033[33m[Active Notice Received]\033[0m\n");
        printf("  ID          : %s\n", notice.id);
        printf("  Severity    : %s\n", severity_name(notice.severity));
        printf("  Title       : %s\n", notice.title);
        printf("  Message     : %s\n", notice.message);
        printf("  Dismissible : %s\n", notice.dismissible ? "Yes" : "No");
        printf("  Deep Link   : %s\n", notice.deep_link[0] ? notice.deep_link : "(none)");
        printf("  Action URL  : %s\n", notice.fallback_url[0] ? notice.fallback_url : "(none)");

        if (do_dismiss) {
            printf("\n--> Executing dismiss notice action...\n");
            nexkeyruntime_dismiss_notice(handle, notice.id, notice.revision);
            printf("    Notice '%s' dismissed for this session.\n", notice.id);
        } else if (do_remind) {
            printf("\n--> Executing remind-later action...\n");
            nexkeyruntime_remind_notice_later(handle, notice.id);
            printf("    Notice postponed.\n");
        }
    } else if (res == NEXKEYRUNTIME_NOT_AVAILABLE) {
        printf("  Notice status: No active notices matching this host/audience.\n");
    } else {
        printf("  Notice status: None available (code %d).\n", (int)res);
    }

    nexkeyruntime_destroy(handle);
    return 0;
}
