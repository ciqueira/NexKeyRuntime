/* Release checks and product notices — the second handle.
 *
 * Independent of the license handle: no license key, no receipt, no
 * activation. See docs/UPDATES_AND_NOTICES.md for the full contract.
 *
 * Not buildable standalone in this repository — see examples/cmake-consumer.
 */

#include <nexkeyruntime/nexkeyruntime.h>

#include <stdio.h>
#include <stddef.h>
#include <string.h>

/* --- opening URLs ------------------------------------------------------ */

/* Return non-zero only when the URL actually opened. The SDK uses the answer
 * to fall back from the mcnexus:// deep link to the https:// one, so a
 * launcher that always claims success leaves users of machines without
 * MCNexus pressing a button that does nothing. */
static int example_open_url(void *user_data, const char *url) {
    (void)user_data;
    printf("  [open] %s\n", url);
    return 1;   /* your platform's launcher goes here */
}

static int example_can_open_url(void *user_data, const char *url) {
    (void)user_data;
    return (url != NULL && url[0] != '\0');
}

/* --- one handle for the whole process ---------------------------------- */

/* NOT one per plugin instance. This handle does not deduplicate itself, so
 * fifty instances holding fifty handles issue fifty requests for the same
 * answer. (The LICENSE handle does share one background refresh — do not
 * carry that assumption over to this one.) */
static NexKeyRuntimeHandle *g_updates = NULL;

static void example_startup(void) {
    NexKeyRuntimeConfig config;
    nexkeyruntime_config_init(&config);

    /* Issued with your ProductData blob during per-project setup. */
    config.tenant_id       = "your-tenant-id";
    config.artifact_id     = "default";
    config.base_url        = "https://your-endpoint";

    /* No defaults — the SDK refuses the config without them. */
    config.current_version = "1.4.2";
    config.platform        = "macos";
    config.architecture    = "arm64";

    config.channel = "stable";
    config.locale  = "pt-BR";

    /* Lets the vendor scope a notice to a host and version. Without these, a
     * notice written for a specific host matches nobody. Pass what the host
     * reports, verbatim. */
    config.host_name    = "YourHostApplication";
    config.host_version = "20.0.1";

    config.can_open_url = example_can_open_url;
    config.open_url     = example_open_url;

    g_updates = nexkeyruntime_create(&config);
    if (g_updates == NULL) {
        /* One of the values above was rejected: a missing required field, a
         * version that does not parse, or a base_url that is not https. */
        fprintf(stderr, "update handle rejected the configuration\n");
        return;
    }

    /* Returns immediately; the work happens in the background. 0 = honour the
     * schedule the backend set. */
    nexkeyruntime_request_check(g_updates, 0);
}

/* --- deciding what to show --------------------------------------------- */

typedef enum { SHOW_NOTHING, SHOW_NOTICE, SHOW_UPDATE } ExampleChoice;

/* The two channels never rank against each other inside the SDK, so this is
 * yours to decide. The recommended order is:
 *
 *     critical notice  >  recommended notice  >  update  >  info notice
 *
 * The part that is easy to get wrong: giving any notice blanket precedence
 * is the obvious reading and it is wrong. An informational notice would then
 * hide a release the user wants, and nothing reports that as a problem. Only
 * `recommended` and `critical` outrank an update. */
static ExampleChoice example_choose(int have_notice,
                                    NexKeyRuntimeNoticeSeverity severity,
                                    int has_update) {
    if (have_notice && (severity > NEXKEYRUNTIME_NOTICE_INFO || !has_update)) {
        return SHOW_NOTICE;
    }
    if (has_update) {
        return SHOW_UPDATE;
    }
    return have_notice ? SHOW_NOTICE : SHOW_NOTHING;
}

/* Call from your UI thread whenever you are in a position to act on the
 * answer — a panel opening, a control changing, a timer if your host has
 * one. Never from a render callback. Reading costs nothing and never blocks
 * on the network: it reports whatever the last completed check produced. */
static void example_refresh_ui(void) {
    if (g_updates == NULL) {
        return;
    }

    NexKeyRuntimeNotice notice;
    memset(&notice, 0, sizeof(notice));
    notice.struct_size = sizeof(notice);
    const int have_notice =
        nexkeyruntime_get_active_notice(g_updates, &notice) == NEXKEYRUNTIME_OK &&
        notice.available;
    /* NOT_AVAILABLE here is the normal case, not an error. */

    NexKeyRuntimeUpdateSnapshot snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.struct_size = sizeof(snapshot);
    const int have_snapshot =
        nexkeyruntime_get_snapshot(g_updates, &snapshot) == NEXKEYRUNTIME_OK;
    const int has_update = have_snapshot && snapshot.has_update;

    switch (example_choose(have_notice, notice.severity, has_update)) {
    case SHOW_NOTICE:
        /* title and message arrive already resolved to config.locale. */
        printf("notice: %s\n  %s\n", notice.title, notice.message);
        printf("  severity=%d type=%d dismissible=%d\n",
               (int)notice.severity, (int)notice.type, notice.dismissible);
        break;

    case SHOW_UPDATE:
        printf("update: %s available (running %s)\n",
               snapshot.latest_version, snapshot.current_version);
        if (snapshot.notes_url[0] != '\0') {
            printf("  release notes: %s\n", snapshot.notes_url);
        }
        break;

    case SHOW_NOTHING:
        /* Up to date, still checking, offline, or the check failed. None of
         * these is worth putting in front of a user: an update check that
         * could not reach the network is not news anyone can act on. */
        break;
    }

    /* A check still running reports CHECKING and the previous answer stays
     * valid, so on a cold install the first look may simply show nothing. */
    if (have_snapshot && snapshot.status == NEXKEYRUNTIME_UPDATE_CHECKING) {
        printf("(a check is in flight)\n");
    }
}

/* --- acting ------------------------------------------------------------- */

static void example_user_pressed_the_button(void) {
    NexKeyRuntimeNotice notice;
    memset(&notice, 0, sizeof(notice));
    notice.struct_size = sizeof(notice);

    if (nexkeyruntime_get_active_notice(g_updates, &notice) == NEXKEYRUNTIME_OK &&
        notice.available) {
        nexkeyruntime_open_notice_action(g_updates, notice.id);
    } else {
        nexkeyruntime_open_update_action(g_updates);
    }
    /* Whichever item you SHOWED is the one to open. Deciding again here with
     * different logic than example_choose() is how a panel ends up saying
     * "version 1.5 available" and opening an unrelated maintenance notice. */
}

static void example_user_dismissed_the_notice(const NexKeyRuntimeNotice *notice) {
    /* The revision matters: a corrected notice republished at a higher
     * revision comes back, which is what makes corrections reach people who
     * dismissed the earlier wording. */
    nexkeyruntime_dismiss_notice(g_updates, notice->id, notice->revision);

    /* Or, to hide it for the period the vendor set on it: */
    /* nexkeyruntime_remind_notice_later(g_updates, notice->id); */

    /* Both last only as long as this handle. If a dismissal should survive a
     * restart, store it yourself and skip the notice on the way to your UI. */
}

static void example_shutdown(void) {
    /* When the plug-in unloads, not when an instance closes. Returns only
     * once any check in flight has been cancelled and finished. */
    nexkeyruntime_destroy(g_updates);
    g_updates = NULL;
}

int main(void) {
    example_startup();
    example_refresh_ui();
    example_user_pressed_the_button();

    NexKeyRuntimeNotice notice;
    memset(&notice, 0, sizeof(notice));
    notice.struct_size = sizeof(notice);
    if (nexkeyruntime_get_active_notice(g_updates, &notice) == NEXKEYRUNTIME_OK &&
        notice.available) {
        example_user_dismissed_the_notice(&notice);
    }

    example_shutdown();
    return 0;
}
