// nexkeyctl: activate, deactivate and inspect a NexKeyRuntime license from
// the command line, in a process that is NOT the OFX host.
//
// It exists for P11. The question that item asks is not "does licensing
// work" but something narrower and nastier: when one process writes the
// receipt and a DIFFERENT process reads it from inside DaVinci Resolve, do
// the two agree on where that file lives? A Resolve installed from the Mac
// App Store runs sandboxed, and a sandboxed process's
// ~/Library/Application Support is silently redirected into the app's own
// container, so the receipt this tool writes would simply not be there
// (§3.12). `--status` prints the RESOLVED path precisely so it can be
// compared against the path the plugin reports from inside the host: two
// different strings is the answer P11 is looking for.
//
// It is also, incidentally, a minimal stand-in for what MCNexus does: the
// same Perfil B calls, in a separate process from the plugin.
//
// --updates covers the OTHER handle, for a different reason: not that a second
// process is needed, but that NO process consumes it. The ColorEqualizer only
// creates the license handle, so without this command the update/notice half
// of the ABI could only ever be exercised against a unit-test fixture.

#include <nexkeyruntime/nexkeyruntime.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace {

// Baked in so the common case needs no flag, overridable so the tool stays
// general; it lives in MCSDK/tools and has no business being welded to one
// product. Resolution order: --tenant, then $NEXKEYRUNTIME_TENANT, then this.
//
// Whichever wins is always PRINTED. A tool that silently defaults to a tenant
// and does not say so can deactivate the wrong license and look like it
// worked.
#ifndef NEXKEYCTL_DEFAULT_TENANT
#define NEXKEYCTL_DEFAULT_TENANT "mcnexus-sandbox"
#endif

const char *statusName(NexKeyRuntimeLicenseStatus status) {
  switch (status) {
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
  }
  return "?";
}

const char *resultName(NexKeyRuntimeResult result) {
  switch (result) {
    case NEXKEYRUNTIME_OK: return "OK";
    case NEXKEYRUNTIME_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
    case NEXKEYRUNTIME_INVALID_CONFIG: return "INVALID_CONFIG";
    // 3-9 reach this tool only through --updates: the licensing calls never
    // return them, which is why they were missing until that command existed
    // and printed a bare "code_5" for an HTTP error.
    case NEXKEYRUNTIME_BUSY: return "BUSY";
    case NEXKEYRUNTIME_NETWORK_ERROR: return "NETWORK_ERROR";
    case NEXKEYRUNTIME_HTTP_ERROR: return "HTTP_ERROR";
    case NEXKEYRUNTIME_INVALID_MANIFEST: return "INVALID_MANIFEST";
    case NEXKEYRUNTIME_UNSUPPORTED_SCHEMA: return "UNSUPPORTED_SCHEMA";
    case NEXKEYRUNTIME_NOT_AVAILABLE: return "NOT_AVAILABLE";
    case NEXKEYRUNTIME_BUFFER_TOO_SMALL: return "BUFFER_TOO_SMALL";
    case NEXKEYRUNTIME_INTERNAL_ERROR: return "INTERNAL_ERROR";
    case NEXKEYRUNTIME_E_PRODUCT_DATA: return "E_PRODUCT_DATA";
    case NEXKEYRUNTIME_E_LICENSE_KEY: return "E_LICENSE_KEY";
    case NEXKEYRUNTIME_E_ACTIVATION_LIMIT: return "E_ACTIVATION_LIMIT";
    case NEXKEYRUNTIME_E_LICENSE_EXPIRED: return "E_LICENSE_EXPIRED";
    case NEXKEYRUNTIME_E_LICENSE_SUSPENDED: return "E_LICENSE_SUSPENDED";
    case NEXKEYRUNTIME_E_LICENSE_REVOKED: return "E_LICENSE_REVOKED";
    case NEXKEYRUNTIME_E_ACTIVATION_NOT_FOUND: return "E_ACTIVATION_NOT_FOUND";
    case NEXKEYRUNTIME_E_STORAGE: return "E_STORAGE";
    default: break;
  }
  static char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "code_%d", static_cast<int>(result));
  return buffer;
}

// The same layout Paths.cpp builds, recomputed here rather than exposed by
// the ABI. Deliberate: the SDK does not publish its storage location, and it
// should not start doing so for a diagnostic. If these two ever disagree that
// is itself worth knowing, which is why --status prints what it CHECKED as
// well as what it found.
std::string receiptsDirectory(const std::string &tenantId) {
#if defined(_WIN32)
  const char *base = std::getenv("LOCALAPPDATA");
  const std::string root = base ? std::string(base) : std::string();
  if (root.empty()) return std::string();
  return root + "\\NexKeyRuntime\\" + tenantId + "\\receipts";
#else
  const char *home = std::getenv("HOME");
  const std::string root = home ? std::string(home) : std::string();
  if (root.empty()) return std::string();
  return root + "/Library/Application Support/NexKeyRuntime/" + tenantId + "/receipts";
#endif
}

// How many receipts are actually sitting there.
//
// NOT "does the directory exist": that would always answer yes and prove
// nothing: resolveTenantPaths(, NEXKEYRUNTIME_SCOPE_USER) creates the whole chain with mkdir -p
// semantics, so by the time any command has run the directory exists no
// matter which path was resolved. A sandboxed host would cheerfully create
// its own redirected copy and look just as healthy.
//
// The signal for P11 is the PATH STRING plus this count: a plugin that
// resolves a different path finds zero receipts there, while the tool that
// wrote one still sees it at the original path. Two paths, two counts, and
// the redirection is visible.
int receiptCount(const std::string &path) {
  if (path.empty()) return -1;
  std::error_code ec;
  if (!std::filesystem::is_directory(path, ec) || ec) return -1;
  int found = 0;
  for (const auto &entry : std::filesystem::directory_iterator(path, ec)) {
    const std::string name = entry.path().filename().string();
    if (name.size() > 5 && name.compare(name.size() - 5, 5, ".json") == 0) {
      ++found;
    }
  }
  return ec ? -1 : found;
}

std::string readFileText(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << in.rdbuf();
  std::string text = buffer.str();
  while (!text.empty() &&
         (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
    text.pop_back();
  }
  return text;
}

const char *argumentValue(int argc, char **argv, const char *name) {
  const std::size_t length = std::strlen(name);
  for (int i = 1; i < argc; ++i) {
    if (std::strncmp(argv[i], name, length) == 0 && argv[i][length] == '=') {
      return argv[i] + length + 1;
    }
  }
  return nullptr;
}

bool hasFlag(int argc, char **argv, const char *name) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], name) == 0) return true;
  }
  return false;
}

const char *updateStatusName(NexKeyRuntimeUpdateStatus status) {
  switch (status) {
    case NEXKEYRUNTIME_UPDATE_IDLE: return "IDLE";
    case NEXKEYRUNTIME_UPDATE_CHECKING: return "CHECKING";
    case NEXKEYRUNTIME_UPDATE_UP_TO_DATE: return "UP_TO_DATE";
    case NEXKEYRUNTIME_UPDATE_AVAILABLE: return "AVAILABLE";
    case NEXKEYRUNTIME_UPDATE_OFFLINE: return "OFFLINE";
    case NEXKEYRUNTIME_UPDATE_ERROR: return "ERROR";
  }
  return "?";
}

const char *noticeTypeName(NexKeyRuntimeNoticeType type) {
  switch (type) {
    case NEXKEYRUNTIME_NOTICE_UPDATE: return "update";
    case NEXKEYRUNTIME_NOTICE_COMPATIBILITY: return "compatibility";
    case NEXKEYRUNTIME_NOTICE_SECURITY: return "security";
    case NEXKEYRUNTIME_NOTICE_MAINTENANCE: return "maintenance";
    case NEXKEYRUNTIME_NOTICE_MIGRATION: return "migration";
    case NEXKEYRUNTIME_NOTICE_DEPRECATION: return "deprecation";
  }
  return "?";
}

const char *noticeSeverityName(NexKeyRuntimeNoticeSeverity severity) {
  switch (severity) {
    case NEXKEYRUNTIME_NOTICE_INFO: return "info";
    case NEXKEYRUNTIME_NOTICE_RECOMMENDED: return "recommended";
    case NEXKEYRUNTIME_NOTICE_CRITICAL: return "critical";
  }
  return "?";
}

// The update handle has no defaults for these two; copyConfig() rejects a
// config that omits either, so the tool supplies the machine it is running
// on and lets --platform/--architecture override. Overriding is a real use:
// asking what a Windows client would be offered, from a Mac, is exactly the
// kind of question this tool exists to answer.
const char *defaultPlatform() {
#if defined(_WIN32)
  return "windows";
#elif defined(__APPLE__)
  return "macos";
#else
  return "";
#endif
}

const char *defaultArchitecture() {
#if defined(__aarch64__) || defined(_M_ARM64)
  return "arm64";
#else
  return "x86_64";
#endif
}

// Updates and notices, in a process that is not the OFX host: the same
// reason the licensing commands above exist, for the other handle.
//
// It is what makes I2/I3 of E2E_VALIDATION.md runnable at all: nothing else
// consumes this half of the ABI today. The ColorEqualizer only ever creates
// the LICENSE handle, so until this command existed the update path had
// unit coverage over a fixture and no way to be exercised against a real
// gateway response.
//
// No ProductData here, deliberately: the update handle does not take one.
// Manifests are public and unsigned by design (SPEC_UPDATES_NOTICES.md);
// they say "0.1.2 exists", which is not a claim that needs a signature. The
// certificate keyring belongs to the licensing handle, and keeping the two
// apart is what lets a plugin ship updates without licensing, or the
// reverse.
int runUpdates(int argc, char **argv, const char *tenant,
               const char *tenantSource) {
  NexKeyRuntimeConfig config;
  nexkeyruntime_config_init(&config);

  const char *artifact = argumentValue(argc, argv, "--artifact");
  const char *platform = argumentValue(argc, argv, "--platform");
  const char *architecture = argumentValue(argc, argv, "--architecture");
  const char *channel = argumentValue(argc, argv, "--channel");
  const char *currentVersion = argumentValue(argc, argv, "--current-version");
  const char *baseUrl = argumentValue(argc, argv, "--base-url");
  const char *locale = argumentValue(argc, argv, "--locale");
  const char *hostName = argumentValue(argc, argv, "--host-name");
  const char *hostVersion = argumentValue(argc, argv, "--host-version");
  const char *timeoutText = argumentValue(argc, argv, "--timeout-ms");

  if (!currentVersion || currentVersion[0] == '\0') {
    std::fprintf(stderr,
                 "nexkeyctl: --updates needs --current-version=X.Y.Z (the version "
                 "you are pretending to be)\n");
    return 2;
  }

  config.tenant_id = tenant;
  config.current_version = currentVersion;
  config.platform = (platform && platform[0] != '\0') ? platform : defaultPlatform();
  config.architecture =
      (architecture && architecture[0] != '\0') ? architecture : defaultArchitecture();
  if (artifact && artifact[0] != '\0') config.artifact_id = artifact;
  if (channel && channel[0] != '\0') config.channel = channel;
  if (baseUrl && baseUrl[0] != '\0') config.base_url = baseUrl;
  if (locale && locale[0] != '\0') config.locale = locale;
  if (hostName && hostName[0] != '\0') config.host_name = hostName;
  if (hostVersion && hostVersion[0] != '\0') config.host_version = hostVersion;
  if (timeoutText && timeoutText[0] != '\0') {
    config.request_timeout_ms =
        static_cast<uint32_t>(std::strtoul(timeoutText, nullptr, 10));
  }

  // Printed BEFORE nexkeyruntime_create, because create() answers a rejected
  // config with a null pointer and no reason. For a diagnostic that is the
  // worst possible pairing: the operator sees "could not create" and cannot
  // tell which field was refused. Showing the resolved values first makes the
  // culprit obvious: an empty platform on Linux, a --current-version that is
  // not a version, a --base-url that is not https.
  std::printf("tenant          : %s  (%s)\n", tenant, tenantSource);
  std::printf("artifact        : %s\n", config.artifact_id);
  std::printf("target          : %s/%s  channel=%s\n", config.platform,
              config.architecture, config.channel);
  std::printf("current version : %s\n", config.current_version);
  std::printf("base url        : %s\n", config.base_url);
  if (config.host_name || config.host_version) {
    std::printf("host            : %s %s\n",
                config.host_name ? config.host_name : "(unnamed)",
                config.host_version ? config.host_version : "");
  }

  NexKeyRuntimeHandle *handle = nexkeyruntime_create(&config);
  if (!handle) {
    std::fprintf(stderr,
                 "nexkeyctl: could not create an update handle: one of the "
                 "values above was rejected\n");
    return 1;
  }

  // force=1 always. Without it the handle honours the manifest's
  // checkAfterSeconds and answers OK without going anywhere, which for a
  // one-shot command reads as a successful check that fetched nothing.
  const NexKeyRuntimeResult started = nexkeyruntime_request_check(handle, 1);
  if (started != NEXKEYRUNTIME_OK) {
    std::fprintf(stderr, "nexkeyctl: request_check failed (%s)\n",
                 resultName(started));
    nexkeyruntime_destroy(handle);
    return 1;
  }

  // The check runs on a worker thread and there is no completion callback in
  // the ABI, so a one-shot process has to poll the snapshot until the status
  // leaves CHECKING. The deadline is the request timeout plus slack: past
  // that, the interesting output is the timeout itself, not a longer wait.
  NexKeyRuntimeUpdateSnapshot snapshot{};
  snapshot.struct_size = sizeof(snapshot);
  const auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(config.request_timeout_ms + 3000);
  bool settled = false;
  while (std::chrono::steady_clock::now() < deadline) {
    snapshot.struct_size = sizeof(snapshot);
    if (nexkeyruntime_get_snapshot(handle, &snapshot) == NEXKEYRUNTIME_OK &&
        snapshot.status != NEXKEYRUNTIME_UPDATE_CHECKING) {
      settled = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
  if (!settled) {
    std::fprintf(stderr, "nexkeyctl: the check did not finish in time\n");
    nexkeyruntime_destroy(handle);
    return 1;
  }

  std::printf("check           : %s\n", updateStatusName(snapshot.status));
  std::printf("last result     : %s\n", resultName(snapshot.last_result));
  std::printf("has update      : %s\n", snapshot.has_update ? "yes" : "no");
  if (snapshot.latest_version[0] != '\0') {
    std::printf("latest version  : %s\n", snapshot.latest_version);
  }
  if (snapshot.release_id[0] != '\0') {
    std::printf("release id      : %s\n", snapshot.release_id);
  }
  if (snapshot.notes_url[0] != '\0') {
    std::printf("notes url       : %s\n", snapshot.notes_url);
  }
  if (snapshot.deep_link[0] != '\0') {
    std::printf("deep link       : %s\n", snapshot.deep_link);
  }
  if (snapshot.fallback_url[0] != '\0') {
    std::printf("fallback url    : %s\n", snapshot.fallback_url);
  }
  // The manifest's checkAfterSeconds, made visible: this is the one field
  // that says whether the server's polling policy actually reached the
  // client, and it is invisible in the manifest body alone.
  if (snapshot.has_checked) {
    std::printf("next check in   : %lld s\n",
                static_cast<long long>(snapshot.next_check_at - snapshot.checked_at));
  }

  NexKeyRuntimeNotice notice{};
  notice.struct_size = sizeof(notice);
  const NexKeyRuntimeResult noticeResult =
      nexkeyruntime_get_active_notice(handle, &notice);
  if (noticeResult == NEXKEYRUNTIME_OK && notice.available) {
    std::printf("notice          : %s  (%s, %s)\n", notice.id,
                noticeTypeName(notice.type), noticeSeverityName(notice.severity));
    std::printf("notice title    : %s\n", notice.title);
    std::printf("notice message  : %s\n", notice.message);
    std::printf("notice revision : %d  dismissible=%s\n", notice.revision,
                notice.dismissible ? "yes" : "no");
    if (notice.deep_link[0] != '\0') {
      std::printf("notice link     : %s\n", notice.deep_link);
    }

    // Both act on THIS handle and nothing else: dismissals and snoozes live
    // in the handle's memory and are never written to disk, so they die with
    // the process. That makes them untestable across runs and perfectly
    // testable within one: show the notice, dismiss it, watch the evaluator
    // drop it. Anything more would need the integrator to persist the state,
    // which is where that responsibility sits today.
    const bool dismiss = hasFlag(argc, argv, "--dismiss");
    const bool remindLater = hasFlag(argc, argv, "--remind-later");
    if (dismiss || remindLater) {
      const NexKeyRuntimeResult acted =
          dismiss ? nexkeyruntime_dismiss_notice(handle, notice.id, notice.revision)
                  : nexkeyruntime_remind_notice_later(handle, notice.id);
      std::printf("%-16s: %s\n", dismiss ? "dismiss" : "remind later",
                  resultName(acted));

      NexKeyRuntimeNotice after{};
      after.struct_size = sizeof(after);
      const NexKeyRuntimeResult again =
          nexkeyruntime_get_active_notice(handle, &after);
      std::printf("notice now      : %s\n",
                  (again == NEXKEYRUNTIME_OK && after.available) ? after.id
                                                                 : "(none)");
      std::printf("                  (in-memory only; gone on the next run)\n");
    }
  } else if (noticeResult == NEXKEYRUNTIME_NOT_AVAILABLE) {
    // Not a failure. The fixture notice carries a narrow audience on
    // purpose, so being filtered out is the expected answer for most
    // invocations: see E2E_VALIDATION.md, I3.
    std::printf("notice          : none for this audience\n");
  } else {
    std::printf("notice          : %s\n", resultName(noticeResult));
  }

  nexkeyruntime_destroy(handle);
  return (snapshot.status == NEXKEYRUNTIME_UPDATE_UP_TO_DATE ||
          snapshot.status == NEXKEYRUNTIME_UPDATE_AVAILABLE)
             ? 0
             : 1;
}

void usage() {
  std::printf(
      "nexkeyctl %s: NexKeyRuntime license and update control\n\n"
      "  nexkeyctl --activate       --key=KEY [--tenant=ID] [--product-file=PATH] [--variant=SLUG]\n"
      "  nexkeyctl --deactivate     [--tenant=ID] [--product-file=PATH]\n"
      "  nexkeyctl --status         [--tenant=ID] [--product-file=PATH]\n"
      "  nexkeyctl --sync           [--tenant=ID] [--product-file=PATH]\n"
      "  nexkeyctl --export-request      --out=PATH [--key=KEY] [--tenant=ID] [--product-file=PATH] [--variant=SLUG]\n"
      "  nexkeyctl --import              --response=PATH [--tenant=ID] [--product-file=PATH]\n"
      "  nexkeyctl --export-deactivation --out=PATH [--tenant=ID] [--product-file=PATH]\n"
      "  nexkeyctl --updates        --current-version=X.Y.Z [--tenant=ID] [--artifact=ID]\n"
      "                             [--platform=macos|windows] [--architecture=arm64|x86_64]\n"
      "                             [--channel=stable|beta|nightly] [--base-url=URL]\n"
      "                             [--locale=L] [--host-name=N] [--host-version=X.Y.Z]\n"
      "                             [--timeout-ms=N] [--dismiss|--remind-later]\n\n"
      "  --tenant        Defaults to $NEXKEYRUNTIME_TENANT, then \"%s\".\n"
      "                  Whichever is used is printed on every command.\n"
      "  --product-file  ProductData blob issued by the backoffice.\n"
      "                  Defaults to $NEXKEYRUNTIME_PRODUCT_FILE.\n"
      "  --variant       Entitlement to require (default: download:default).\n\n"
      "--status prints the receipt directory this process resolved. Compare it\n"
      "against the path the OFX plugin reports from inside the host: if they\n"
      "differ, the host is sandboxed and redirecting storage (P11, §3.12).\n\n"
      "--export-request writes a JSON file identifying this machine (tenant,\n"
      "machineBinding, entitlement, a prefix of --key if given) so someone else\n"
      "can issue an activation certificate for it without this machine reaching\n"
      "the network. --import takes what comes back (the certificate, a plain\n"
      "signed JSON envelope, not tied to any particular filename) via --response\n"
      "and loads it exactly as a normal sync would; the certificate is verified\n"
      "against the ProductData keyring like any other, on this call and on every\n"
      "load after it; this tool does not weaken that check.\n\n"
      "--export-deactivation gives up the licence on THIS machine and writes the\n"
      "proof of it. The local receipts are deleted first, so the machine stops\n"
      "rendering before the proof exists; that is what makes the file worth\n"
      "acting on without a signature. Send it to whoever administers the licence;\n"
      "they release the seat. Reactivating means a fresh --export-request.\n\n"
      "--updates is the OTHER handle, and shares nothing with the ones above: no\n"
      "ProductData, no receipt, no licence. It asks the gateway what release and\n"
      "what notices apply to the target you describe, and prints the answer. The\n"
      "target is described, not detected, so you can ask what a Windows client on\n"
      "an older version would be told without owning one, which is the only way\n"
      "to test an audience rule that excludes the machine you are sitting at.\n"
      "--dismiss and --remind-later act on the notice just printed, then print\n"
      "what is left; both are in-memory and do not survive the process.\n",
      nexkeyruntime_version(), NEXKEYCTL_DEFAULT_TENANT);
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2 || hasFlag(argc, argv, "--help") || hasFlag(argc, argv, "-h")) {
    usage();
    return argc < 2 ? 2 : 0;
  }

  const bool activate = hasFlag(argc, argv, "--activate");
  const bool deactivate = hasFlag(argc, argv, "--deactivate");
  const bool status = hasFlag(argc, argv, "--status");
  const bool sync = hasFlag(argc, argv, "--sync");
  const bool exportRequest = hasFlag(argc, argv, "--export-request");
  const bool importResponse = hasFlag(argc, argv, "--import");
  const bool exportDeactivation = hasFlag(argc, argv, "--export-deactivation");
  const bool updates = hasFlag(argc, argv, "--updates");
  if (activate + deactivate + status + sync + exportRequest + importResponse +
          exportDeactivation + updates != 1) {
    std::fprintf(stderr,
                 "nexkeyctl: pick exactly one of --activate, --deactivate, --status, --sync, "
                 "--export-request, --import, --export-deactivation, --updates\n");
    return 2;
  }

  const char *tenant = argumentValue(argc, argv, "--tenant");
  const char *tenantSource = "--tenant";
  if (!tenant || tenant[0] == '\0') {
    tenant = std::getenv("NEXKEYRUNTIME_TENANT");
    tenantSource = "$NEXKEYRUNTIME_TENANT";
  }
  if (!tenant || tenant[0] == '\0') {
    tenant = NEXKEYCTL_DEFAULT_TENANT;
    tenantSource = "built-in default";
  }

  // Before --product-file is demanded: the update handle takes none, and
  // requiring one here would make the command impossible to run for a plugin
  // that ships updates without licensing, a combination the ABI explicitly
  // supports.
  if (updates) {
    return runUpdates(argc, argv, tenant, tenantSource);
  }

  const char *productFile = argumentValue(argc, argv, "--product-file");
  if (!productFile) productFile = std::getenv("NEXKEYRUNTIME_PRODUCT_FILE");
  std::string resolvedProductPath;
  if (productFile && productFile[0] != '\0') {
    resolvedProductPath = productFile;
  } else {
    std::error_code ec;
    const char *candidates[] = {
      "sandbox/mcnexus-sandbox.nexkeydat",
      "../sandbox/mcnexus-sandbox.nexkeydat",
      "../../sandbox/mcnexus-sandbox.nexkeydat",
      "../../../sandbox/mcnexus-sandbox.nexkeydat",
      "MCSDK/NexKeyRuntime/sandbox/mcnexus-sandbox.nexkeydat"
    };
    for (const char *cand : candidates) {
      if (std::filesystem::exists(cand, ec) && !ec) {
        resolvedProductPath = cand;
        break;
      }
    }
  }
  if (resolvedProductPath.empty()) {
    std::fprintf(stderr,
                 "nexkeyctl: --product-file (or $NEXKEYRUNTIME_PRODUCT_FILE) is required\n");
    return 2;
  }
  const std::string productData = readFileText(resolvedProductPath);
  if (productData.empty()) {
    std::fprintf(stderr, "nexkeyctl: could not read ProductData from %s\n", resolvedProductPath.c_str());
    return 2;
  }

  const char *variant = argumentValue(argc, argv, "--variant");

  NexKeyRuntimeLicenseHandle *handle = nexkeyruntime_license_create();
  if (!handle) {
    std::fprintf(stderr, "nexkeyctl: could not create a license handle\n");
    return 1;
  }

  NexKeyRuntimeResult step = nexkeyruntime_license_set_product_data(handle, productData.c_str());
  if (step != NEXKEYRUNTIME_OK) {
    std::fprintf(stderr, "nexkeyctl: set_product_data failed (%s)\n", resultName(step));
    nexkeyruntime_license_destroy(handle);
    return 1;
  }
  nexkeyruntime_license_set_tenant_id(handle, tenant);
  if (variant) nexkeyruntime_license_set_variant(handle, variant);
  // nexkeyruntime_license_set_metadata(handle, "appVersion", "nexkeyctl/" NEXKEYRUNTIME_VERSION_STRING);
  nexkeyruntime_license_set_metadata(handle, "appVersion", "v1.0.5");
  // Identify ourselves for the same reason an integrator should: an activation
  // is shared by every program on the machine holding that entitlement, and
  // one that declares nothing is recorded as "unknown", indistinguishable
  // from any other silent client. This tool asked ColorEqualizer to do it, so
  // it does it too.
  nexkeyruntime_license_set_metadata(handle, "product", "nexkeyctl");

  // No background poller: this is a one-shot command that exits immediately,
  // and a thread started here would be stopped microseconds later without
  // ever completing a sync. --sync does the work synchronously instead.
  nexkeyruntime_license_set_headless(handle, 1);

  NexKeyRuntimeResult result = NEXKEYRUNTIME_OK;
  std::string exportedRequestPath;
  if (activate) {
    const char *key = argumentValue(argc, argv, "--key");
    if (!key || key[0] == '\0') {
      std::fprintf(stderr, "nexkeyctl: --activate needs --key=KEY\n");
      nexkeyruntime_license_destroy(handle);
      return 2;
    }
    nexkeyruntime_license_set_license_key(handle, key);
    result = nexkeyruntime_license_activate(handle);
  } else if (deactivate) {
    result = nexkeyruntime_license_deactivate(handle);
  } else if (sync) {
    nexkeyruntime_license_load_local(handle);
    result = nexkeyruntime_license_request_sync(handle, 1);
  } else if (exportRequest) {
    const char *outPath = argumentValue(argc, argv, "--out");
    if (!outPath || outPath[0] == '\0') {
      std::fprintf(stderr, "nexkeyctl: --export-request needs --out=PATH\n");
      nexkeyruntime_license_destroy(handle);
      return 2;
    }
    // Optional here, unlike --activate: exporting a request for a machine
    // where no key has been entered yet is a real case (the operator asks
    // the customer for it separately), and an empty prefix is a valid,
    // honest answer, not an error.
    const char *key = argumentValue(argc, argv, "--key");
    if (key && key[0] != '\0') {
      nexkeyruntime_license_set_license_key(handle, key);
    }
    result = nexkeyruntime_license_export_activation_request(handle, outPath);
    if (result == NEXKEYRUNTIME_OK) {
      exportedRequestPath = outPath;
    }
  } else if (exportDeactivation) {
    const char *outPath = argumentValue(argc, argv, "--out");
    if (!outPath || outPath[0] == '\0') {
      std::fprintf(stderr, "nexkeyctl: --export-deactivation needs --out=PATH\n");
      nexkeyruntime_license_destroy(handle);
      return 2;
    }
    // Gives up the licence on THIS machine and writes the proof. Irreversible
    // locally: reactivating means a fresh --export-request round trip.
    result = nexkeyruntime_license_export_deactivation_proof(handle, outPath);
    if (result == NEXKEYRUNTIME_OK) {
      exportedRequestPath = outPath;
    }
  } else if (importResponse) {
    const char *responsePath = argumentValue(argc, argv, "--response");
    if (!responsePath || responsePath[0] == '\0') {
      std::fprintf(stderr, "nexkeyctl: --import needs --response=PATH\n");
      nexkeyruntime_license_destroy(handle);
      return 2;
    }
    const std::string response = readFileText(responsePath);
    if (response.empty()) {
      std::fprintf(stderr, "nexkeyctl: could not read a response from %s\n", responsePath);
      nexkeyruntime_license_destroy(handle);
      return 1;
    }
    // publish_receipt() verifies this against the ProductData keyring the
    // same way load_local() verifies anything else (§ its own doc comment
    // in nexkeyruntime.h); an invalid or wrong-machine response is
    // rejected here, not silently accepted because it arrived via --import.
    result = nexkeyruntime_license_publish_receipt(handle, response.c_str());
  } else {
    result = nexkeyruntime_license_load_local(handle);
  }

  NexKeyRuntimeLicenseSnapshot snapshot{};
  snapshot.struct_size = sizeof(snapshot);
  const bool haveSnapshot =
      nexkeyruntime_license_get_snapshot(handle, &snapshot) == NEXKEYRUNTIME_OK;
  const NexKeyRuntimeRenderDecision decision =
      nexkeyruntime_license_render_decision(handle);

  std::printf("tenant          : %s  (%s)\n", tenant, tenantSource);
  std::printf("call            : %s\n", resultName(result));
  std::printf("status          : %s\n", haveSnapshot ? statusName(snapshot.status) : "(none)");
  std::printf("render decision : %s\n",
              decision == NEXKEYRUNTIME_RENDER_ALLOW ? "ALLOW" : "DENY");
  if (haveSnapshot && snapshot.activation_id[0] != '\0') {
    std::printf("activation id   : %s\n", snapshot.activation_id);
    std::printf("edition         : %s\n", snapshot.edition);
    if (snapshot.max_activations > 0) {
      std::printf("activations     : %d/%d\n", snapshot.activations_used,
                  snapshot.max_activations);
    } else {
      // Seat counts come from the server, never from the receipt, so a
      // purely local read has no idea. Printing "0/0" would read as "zero
      // seats in use", which is alarming and wrong.
      std::printf("activations     : unknown (run --sync to refresh)\n");
    }
  }
  if (!exportedRequestPath.empty()) {
    std::printf("%-16s: %s\n", exportDeactivation ? "proof file" : "request file", exportedRequestPath.c_str());
  }
  // Printed on every command, not just --status: the whole point of P11 is
  // comparing this line with the one the plugin prints from inside the host.
  const std::string receipts = receiptsDirectory(tenant);
  std::printf("receipts dir    : %s\n", receipts.c_str());
  const int found = receiptCount(receipts);
  if (found < 0) {
    std::printf("receipts found  : directory unreadable\n");
  } else {
    std::printf("receipts found  : %d\n", found);
  }

  nexkeyruntime_license_destroy(handle);
  return result == NEXKEYRUNTIME_OK ? 0 : 1;
}
