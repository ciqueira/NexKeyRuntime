/* Activating a machine that cannot reach the network.
 *
 * Air-gapped installations, facilities that block outbound connections, or a
 * backend that is simply unreachable when the user needs to work. The
 * exchange is files carried by whatever means the site has — a shared folder,
 * a USB stick, an email to support.
 *
 * See docs/OFFLINE.md. Not buildable standalone in this repository — see
 * examples/cmake-consumer.
 */

#include <nexkeyruntime/nexkeyruntime.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

static const char *NEXKEYRUNTIME_PRODUCT_DATA =
    "eyJmb3JtYXQiOiJuZXhrZXlydW50aW1lLXByb2R1Y3RkYXRhLXYxIiwi...";

static NexKeyRuntimeLicenseHandle *open_handle(void) {
    NexKeyRuntimeLicenseHandle *license = nexkeyruntime_license_create();
    nexkeyruntime_license_set_product_data(license, NEXKEYRUNTIME_PRODUCT_DATA);
    nexkeyruntime_license_set_tenant_id(license, "your-tenant-id");
    nexkeyruntime_license_set_variant(license, "download:your-entitlement");
    return license;
}

/* --- step 1: ask ------------------------------------------------------- */

static int export_request(NexKeyRuntimeLicenseHandle *license,
                          const char *user_entered_key,
                          const char *path) {
    /* Optional here, unlike online activation: exporting a request for a
     * machine where the user has not typed a key yet is a real case, because
     * the operator often collects it separately. */
    if (user_entered_key != NULL) {
        nexkeyruntime_license_set_license_key(license, user_entered_key);
    }

    const NexKeyRuntimeResult result =
        nexkeyruntime_license_export_activation_request(license, path);

    switch (result) {
    case NEXKEYRUNTIME_OK:
        /* A small JSON file with this machine's binding, the entitlement and
         * at most a prefix of the key. It holds no secret and is not signed —
         * this machine has no key to sign with. Send it to whoever
         * administers the license. */
        printf("send this file for signing: %s\n", path);
        return 1;
    case NEXKEYRUNTIME_INVALID_CONFIG:
        fprintf(stderr, "set product data and tenant id first\n");
        return 0;
    case NEXKEYRUNTIME_E_STORAGE:
        fprintf(stderr, "could not write %s\n", path);
        return 0;
    default:
        fprintf(stderr, "could not export the request (%d)\n", (int)result);
        return 0;
    }
}

/* --- step 2: install what comes back ----------------------------------- */

static char *read_whole_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size <= 0) {
        fclose(file);
        return NULL;
    }
    char *text = (char *)malloc((size_t)size + 1u);
    if (text == NULL) {
        fclose(file);
        return NULL;
    }
    const size_t read = fread(text, 1u, (size_t)size, file);
    fclose(file);
    text[read] = '\0';
    return text;
}

static int import_certificate(NexKeyRuntimeLicenseHandle *license,
                              const char *path) {
    char *certificate = read_whole_file(path);
    if (certificate == NULL) {
        fprintf(stderr, "could not read %s\n", path);
        return 0;
    }

    /* The certificate is a plain signed document, not tied to a filename.
     * Importing it offline weakens nothing: it is verified against the
     * ProductData keyring here and on every load afterwards, so one issued
     * for a different machine — or altered in transit — is rejected. */
    const NexKeyRuntimeResult result =
        nexkeyruntime_license_publish_receipt(license, certificate);
    free(certificate);

    if (result != NEXKEYRUNTIME_OK) {
        fprintf(stderr, "certificate rejected (%d)\n", (int)result);
        return 0;
    }
    printf("activated: render_decision() now answers ALLOW\n");
    return 1;
}

/* --- step 3: give the seat back ---------------------------------------- */

static void export_deactivation(NexKeyRuntimeLicenseHandle *license,
                                const char *path) {
    const NexKeyRuntimeResult result =
        nexkeyruntime_license_export_deactivation_proof(license, path);

    if (result == NEXKEYRUNTIME_E_NO_RECEIPT) {
        printf("nothing to give up — this machine is not activated\n");
        return;
    }
    if (result != NEXKEYRUNTIME_OK) {
        fprintf(stderr, "could not write the proof (%d)\n", (int)result);
        return;
    }

    /* By the time this file exists the license has ALREADY stopped working
     * here — the local proof is removed first, and that ordering is what
     * makes the file trustworthy without a signature: forging it gains the
     * forger nothing, because their own copy stopped first.
     *
     * Not reversible locally. Getting this machine working again means a
     * fresh request/certificate round trip. */
    printf("send this to release the seat: %s\n", path);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    NexKeyRuntimeLicenseHandle *license = open_handle();

    if (export_request(license, "XXXX-YYYY-ZZZZ-WWWW-VVVV", "request.json")) {
        /* ... the file leaves the machine, a certificate comes back ... */
        if (import_certificate(license, "certificate.json")) {
            export_deactivation(license, "proof.json");
        }
    }

    nexkeyruntime_license_destroy(license);
    return 0;
}
