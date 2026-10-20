// =============================================================================
// MCLicense.h
// -----------------------------------------------------------------------------
// NexKeyRuntime licensing integration helper for OFX plugins and host applications.
//
// This header provides a zero-overhead C++ licensing client that inspects
// locally stored certificates, evaluates render decisions (< 1 ns atomic check),
// and manages background license synchronization.
//
// When MC_NEXKEY_ENABLED is not defined, this header compiles to a lightweight
// no-op stub allowing uninstrumented builds.
// =============================================================================

#ifndef MC_LICENSE_H
#define MC_LICENSE_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#ifdef MC_NEXKEY_ENABLED
#include <nexkeyruntime/nexkeyruntime.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#if !defined(_WIN32)
#include <dirent.h>
#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#endif

namespace mc {

// Diagnostic report populated by License::report() for UI and telemetry.
struct LicenseReport {
  std::string status = "disabled";
  std::string edition;
  std::string validity;
  std::string activation;
  std::string receiptsDir;
  std::string sync;
  int receiptsFound = -1;
  int daysRemaining = 0;

  bool allowed = false;
  bool enforcing = false;
};

// Plugin classification for active license edition.
enum class ReceiptEdition { Full, Demo, Trial, Beta, Unknown };

#ifdef MC_NEXKEY_ENABLED

// Enforcement is enabled by default. Set MC_NEXKEY_ENFORCE=0 for shadow mode
// (diagnostics reported, but render allowed regardless).
#ifndef MC_NEXKEY_ENFORCE
#define MC_NEXKEY_ENFORCE 1
#endif

#ifndef MC_NEXKEY_TENANT
#define MC_NEXKEY_TENANT "mcnexus-sandbox"
#endif

#ifndef MC_NEXKEY_SCOPE
#define MC_NEXKEY_SCOPE NEXKEYRUNTIME_SCOPE_USER
#endif

#ifndef MC_NEXKEY_VARIANT
#define MC_NEXKEY_VARIANT "default"
#endif

#ifndef MC_NEXKEY_PRODUCT
#define MC_NEXKEY_PRODUCT "mcnexus-playground-plugin"
#endif

#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "1.0.0"
#endif

// Resolves the signed ProductData blob. Falls back to environment variables
// or the default MCNexus Sandbox trust root.
inline std::string productDataBlob() {
#ifdef MC_NEXKEY_PRODUCT_DATA
  return std::string(MC_NEXKEY_PRODUCT_DATA);
#else
  const char *fromEnv = std::getenv("NEXKEYRUNTIME_PRODUCT_DATA");
  if (fromEnv && fromEnv[0] != '\0') return std::string(fromEnv);
  const char *fromFile = std::getenv("NEXKEYRUNTIME_PRODUCT_FILE");
  if (fromFile && fromFile[0] != '\0') {
    if (FILE *handle = std::fopen(fromFile, "rb")) {
      std::string text;
      char buffer[1024];
      std::size_t read = 0;
      while ((read = std::fread(buffer, 1, sizeof(buffer), handle)) > 0) {
        text.append(buffer, read);
      }
      std::fclose(handle);
      while (!text.empty() && (text.back() == '\n' || text.back() == '\r' ||
                               text.back() == ' ')) {
        text.pop_back();
      }
      return text;
    }
  }
  // Default MCNexus Developer Sandbox ProductData envelope
  return "eyJmb3JtYXQiOiJuZXhrZXlydW50aW1lLXByb2R1Y3RkYXRhLXYxIiwia2V5SWQiOiJtY25leHVzLXNhbmRib3gtc2FuZGJveC0yMDI2LTEwIiwicGF5bG9hZCI6ImV5SmlZWE5sVlhKc0lqb2lhSFIwY0hNNkx5OXpaR3N0YzJGdVpHSnZlQzV0WTI1bGVIVnpMbUZ3Y0NJc0ltWnZjbTFoZEZabGNuTnBiMjRpT2pFc0ltbHpjM1ZsWkVGMElqb3hOemt5TkRVek1UYzVMQ0pyWlhseWFXNW5JanBiZXlKclpYbEpaQ0k2SW0xamJtVjRkWE10YzJGdVpHSnZlQzF6WVc1a1ltOTRMVEl3TWpZdE1UQWlMQ0p3ZFdKc2FXTkxaWGtpT2lKbGRIZDRibGxPTFc5UU5sTmtXbFIzVURCMGIyaE1lRzAwWlUweE9WbHpORzl0ZEdock4xQjFZVTUzSW4xZExDSnViM1JCWm5SbGNpSTZNSDAiLCJzaWduYXR1cmUiOiJ5dVZ0bWJ3WXJpODVxQTMyOXhfMVhWWjE5bl9JTkl3RWlGUWlrQkgyV3AxNndIUkhsOWtiM2R3R0JJbm50WDMxdHVDcElzeEppdnIybHZ3RVc3Z25EQSJ9";
#endif
}

inline const char *editionName(NexKeyRuntimeEdition edition) {
  switch (edition) {
    case NEXKEYRUNTIME_EDITION_DEMO: return "demo";
    case NEXKEYRUNTIME_EDITION_TRIAL: return "trial";
    case NEXKEYRUNTIME_EDITION_BETA: return "beta";
    case NEXKEYRUNTIME_EDITION_FULL: return "full";
    case NEXKEYRUNTIME_EDITION_UNKNOWN: break;
  }
  return "UNKNOWN";
}

inline ReceiptEdition toReceiptEdition(NexKeyRuntimeEdition edition) {
  switch (edition) {
    case NEXKEYRUNTIME_EDITION_FULL: return ReceiptEdition::Full;
    case NEXKEYRUNTIME_EDITION_DEMO: return ReceiptEdition::Demo;
    case NEXKEYRUNTIME_EDITION_TRIAL: return ReceiptEdition::Trial;
    case NEXKEYRUNTIME_EDITION_BETA: return ReceiptEdition::Beta;
    case NEXKEYRUNTIME_EDITION_UNKNOWN: break;
  }
  return ReceiptEdition::Demo;
}

// Synchronous one-shot read for host describe/load actions.
inline ReceiptEdition editionAtLoad() {
  NexKeyRuntimeLicenseHandle *handle = nexkeyruntime_license_create();
  if (!handle) return ReceiptEdition::Unknown;
  ReceiptEdition result = ReceiptEdition::Unknown;
  const std::string blob = productDataBlob();
  if (!blob.empty() &&
      nexkeyruntime_license_set_product_data(handle, blob.c_str()) ==
          NEXKEYRUNTIME_OK) {
    nexkeyruntime_license_set_scope(handle, MC_NEXKEY_SCOPE);
    nexkeyruntime_license_set_tenant_id(handle, MC_NEXKEY_TENANT);
    nexkeyruntime_license_set_variant(handle, MC_NEXKEY_VARIANT);
    nexkeyruntime_license_load_local(handle);
    NexKeyRuntimeLicenseSnapshot snapshot{};
    snapshot.struct_size = sizeof(snapshot);
    if (nexkeyruntime_license_get_snapshot(handle, &snapshot) ==
        NEXKEYRUNTIME_OK) {
      result = (snapshot.activation_id[0] == '\0' || snapshot.status == NEXKEYRUNTIME_LICENSE_NOT_ACTIVATED)
                   ? ReceiptEdition::Unknown
                   : toReceiptEdition(snapshot.edition_enum);
    }
  }
  nexkeyruntime_license_destroy(handle);
  return result;
}

inline std::string formatTime(std::int64_t unixSeconds) {
  if (unixSeconds <= 0) return "-";
  const std::time_t raw = static_cast<std::time_t>(unixSeconds);
  std::tm parts{};
#if defined(_WIN32)
  localtime_s(&parts, &raw);
#else
  localtime_r(&raw, &parts);
#endif
  char buffer[32];
  if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &parts) == 0) {
    return "-";
  }
  return std::string(buffer);
}

inline const char *statusName(NexKeyRuntimeLicenseStatus status) {
  switch (status) {
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
    case NEXKEYRUNTIME_LICENSE_UNKNOWN: break;
  }
  return "UNKNOWN";
}

inline std::string receiptsDirectory() {
#if defined(_WIN32)
  const char *base = std::getenv("PROGRAMDATA");
  if (!base || base[0] == '\0') return std::string();
  return std::string(base) + "\\NexKeyRuntime\\" MC_NEXKEY_TENANT "\\receipts";
#else
  const char *home = std::getenv("HOME");
  if (!home || home[0] == '\0') return std::string();
  return std::string(home) +
         "/Library/Application Support/NexKeyRuntime/" MC_NEXKEY_TENANT "/receipts";
#endif
}

inline int countReceipts(const std::string &path) {
  if (path.empty()) return -1;
#if defined(_WIN32)
  WIN32_FIND_DATAA data{};
  const std::string pattern = path + "\\*.json";
  HANDLE find = FindFirstFileA(pattern.c_str(), &data);
  if (find == INVALID_HANDLE_VALUE) return 0;
  int found = 0;
  do { ++found; } while (FindNextFileA(find, &data));
  FindClose(find);
  return found;
#else
  DIR *dir = ::opendir(path.c_str());
  if (!dir) return -1;
  int found = 0;
  while (struct dirent *entry = ::readdir(dir)) {
    const std::string name(entry->d_name);
    if (name.size() > 5 && name.compare(name.size() - 5, 5, ".json") == 0) {
      ++found;
    }
  }
  ::closedir(dir);
  return found;
#endif
}

// Manages the licensing handle lifecycle for an OFX plugin or standalone tool.
class License {
public:
  License() = default;
  ~License() { shutdown(); }
  License(const License &) = delete;
  License &operator=(const License &) = delete;

  // Initialize licensing and start background sync threads.
  void start() {
    if (handle_) return;
    handle_ = nexkeyruntime_license_create();
    if (!handle_) return;

    nexkeyruntime_license_set_callback(handle_, &License::onSync, this);

    const std::string blob = productDataBlob();
    if (blob.empty() ||
        nexkeyruntime_license_set_product_data(handle_, blob.c_str()) !=
            NEXKEYRUNTIME_OK) {
      configured_ = false;
      return;
    }
    nexkeyruntime_license_set_scope(handle_, MC_NEXKEY_SCOPE);
    nexkeyruntime_license_set_tenant_id(handle_, MC_NEXKEY_TENANT);
    nexkeyruntime_license_set_variant(handle_, MC_NEXKEY_VARIANT);
    nexkeyruntime_license_set_metadata(handle_, "appVersion", PLUGIN_VERSION);
    nexkeyruntime_license_set_metadata(handle_, "product", MC_NEXKEY_PRODUCT);
    configured_ = true;
    refresh();
    startRecheckLoop();
  }

  // Alias for start()
  void init() { start(); }

  void shutdown() {
    stopRecheck_.store(true, std::memory_order_release);
    recheckCv_.notify_all();
    if (recheckWorker_.joinable()) recheckWorker_.join();

    if (syncWorker_.joinable()) syncWorker_.join();
    if (!handle_) return;

    nexkeyruntime_license_destroy(handle_);
    handle_ = nullptr;
  }

  // Reloads receipt from local storage.
  void refresh() {
    if (!handle_ || !configured_) return;
    nexkeyruntime_license_load_local(handle_);
    lastLocalReadAt_.store(static_cast<long long>(std::time(nullptr)),
                           std::memory_order_release);

    NexKeyRuntimeLicenseSnapshot snapshot{};
    snapshot.struct_size = sizeof(snapshot);
    if (nexkeyruntime_license_get_snapshot(handle_, &snapshot) == NEXKEYRUNTIME_OK) {
      const ReceiptEdition next =
          (snapshot.activation_id[0] == '\0' || snapshot.status == NEXKEYRUNTIME_LICENSE_NOT_ACTIVATED)
              ? ReceiptEdition::Unknown
              : toReceiptEdition(snapshot.edition_enum);
      edition_.store(static_cast<int>(next), std::memory_order_release);
      daysRemaining_.store(snapshot.days_remaining, std::memory_order_release);
    }
  }

  // Rate-limited refresh for parameter change actions.
  void refreshIfStale(int minimumIntervalSeconds = 3) {
    if (!handle_ || !configured_) return;
    const long long now = static_cast<long long>(std::time(nullptr));
    const long long last = lastLocalReadAt_.load(std::memory_order_acquire);
    if (last != 0 && now >= last && now - last < minimumIntervalSeconds) {
      return;
    }
    refresh();
  }

  // Synchronous sync call.
  void syncNow() {
    if (!handle_ || !configured_) return;
    const unsigned long before = syncCount_.load(std::memory_order_acquire);
    const NexKeyRuntimeResult result =
        nexkeyruntime_license_request_sync(handle_, 1);
    manualSyncs_.fetch_add(1, std::memory_order_acq_rel);
    lastSyncOk_.store(result == NEXKEYRUNTIME_OK, std::memory_order_release);

    if (result == NEXKEYRUNTIME_OK) {
      for (int attempt = 0; attempt < 80; ++attempt) {
        if (syncCount_.load(std::memory_order_acquire) != before) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
    }
    lastSyncAt_.store(static_cast<long long>(std::time(nullptr)),
                      std::memory_order_release);
  }

  // Asynchronous sync call for UI button handlers.
  void syncNowAsync() {
    if (!handle_ || !configured_) return;
    bool expected = false;
    if (!syncInFlight_.compare_exchange_strong(expected, true,
                                               std::memory_order_acq_rel)) {
      return;
    }
    if (syncWorker_.joinable()) syncWorker_.join();
    try {
      syncWorker_ = std::thread([this]() {
        const NexKeyRuntimeResult result =
            nexkeyruntime_license_request_sync(handle_, 1);
        manualSyncs_.fetch_add(1, std::memory_order_acq_rel);
        lastSyncOk_.store(result == NEXKEYRUNTIME_OK, std::memory_order_release);
        lastSyncAt_.store(static_cast<long long>(std::time(nullptr)),
                          std::memory_order_release);
        syncInFlight_.store(false, std::memory_order_release);
      });
    } catch (...) {
      syncInFlight_.store(false, std::memory_order_release);
    }
  }

  // High-performance atomic render check (< 1 ns overhead, zero network I/O).
  bool allowed() const {
#if MC_NEXKEY_ENFORCE
    if (!handle_) return true;
    return nexkeyruntime_license_render_decision(handle_) ==
           NEXKEYRUNTIME_RENDER_ALLOW;
#else
    return true;
#endif
  }

  bool licenseAllows() const {
    if (!handle_) return true;
    return nexkeyruntime_license_render_decision(handle_) ==
           NEXKEYRUNTIME_RENDER_ALLOW;
  }

  ReceiptEdition edition() const {
    return static_cast<ReceiptEdition>(edition_.load(std::memory_order_acquire));
  }

  int daysRemaining() const {
    return daysRemaining_.load(std::memory_order_acquire);
  }

  // Returns true if the frame requires a demo watermark overlay.
  bool demoWatermark() const {
    const ReceiptEdition e = edition();
    return e == ReceiptEdition::Demo || e == ReceiptEdition::Unknown;
  }

  bool demoChart() const { return demoWatermark(); }

  LicenseReport report() const {
    LicenseReport out;
    out.enforcing = MC_NEXKEY_ENFORCE ? true : false;
    out.receiptsDir = receiptsDirectory();
    out.receiptsFound = countReceipts(out.receiptsDir);
    out.daysRemaining = daysRemaining();
    if (!handle_) { out.status = "no handle"; return out; }
    if (!configured_) { out.status = "PRODUCT_DATA MISSING/INVALID"; return out; }

    NexKeyRuntimeLicenseSnapshot snapshot{};
    snapshot.struct_size = sizeof(snapshot);
    if (nexkeyruntime_license_get_snapshot(handle_, &snapshot) != NEXKEYRUNTIME_OK) {
      out.status = "no snapshot";
      return out;
    }

    out.status = statusName(snapshot.status);
    out.allowed = licenseAllows();
    out.daysRemaining = snapshot.days_remaining;

    {
      const unsigned long background = syncCount_.load(std::memory_order_acquire);
      const unsigned long manual = manualSyncs_.load(std::memory_order_acquire);
      const unsigned long count = background + manual;
      const long long observedAt = lastSyncAt_.load(std::memory_order_acquire);
      const std::int64_t when =
          snapshot.last_synced_at != 0 ? snapshot.last_synced_at
                                       : static_cast<std::int64_t>(observedAt);
      if (count == 0 && when == 0) {
        out.sync = "none yet";
      } else {
        out.sync = std::to_string(background) + " background, " +
                   std::to_string(manual) + " manual";
        if (when != 0) out.sync += "   last " + formatTime(when);
        const int last = lastSyncStatus_.load(std::memory_order_acquire);
        if (last >= 0) {
          out.sync += "   -> ";
          out.sync += statusName(static_cast<NexKeyRuntimeLicenseStatus>(last));
        } else if (manual != 0) {
          out.sync += lastSyncOk_.load(std::memory_order_acquire) ? "   -> OK"
                                                                 : "   -> FAILED";
        }
      }
    }

    if (snapshot.activation_id[0] == '\0') {
      out.edition = "-";
      out.validity = "-";
      out.activation = "-";
      return out;
    }

    out.edition = snapshot.edition[0] ? snapshot.edition : "(none)";
    out.edition += "  [";
    out.edition += editionName(snapshot.edition_enum);
    out.edition += "]";

    out.validity = snapshot.expires_at == 0
                       ? std::string("perpetual")
                       : (formatTime(snapshot.expires_at) + "  (" +
                          std::to_string(snapshot.days_remaining) + " day(s) left)");
    if (snapshot.offline_valid_until != 0) {
      out.validity += "   offline until " + formatTime(snapshot.offline_valid_until);
    }
    if (snapshot.sync_after != 0) {
      out.validity += "   next sync " + formatTime(snapshot.sync_after);
    }

    out.activation = snapshot.activation_id[0] ? snapshot.activation_id : "(none)";
    if (snapshot.max_activations > 0) {
      out.activation += "   seats " + std::to_string(snapshot.activations_used) +
                        "/" + std::to_string(snapshot.max_activations);
    } else {
      out.activation += "   seats unknown (sync to refresh)";
    }
    return out;
  }

private:
  static void onSync(NexKeyRuntimeLicenseStatus status, void *userData) {
    if (License *self = static_cast<License *>(userData)) {
      self->syncCount_.fetch_add(1, std::memory_order_acq_rel);
      self->lastSyncStatus_.store(static_cast<int>(status), std::memory_order_release);
      self->lastSyncAt_.store(static_cast<long long>(std::time(nullptr)),
                              std::memory_order_release);
    }
  }

  void startRecheckLoop() {
    if (!handle_ || !configured_) return;
    stopRecheck_.store(false, std::memory_order_release);
    try {
      recheckWorker_ = std::thread([this]() {
        std::unique_lock<std::mutex> lock(recheckMutex_);
        while (!stopRecheck_.load(std::memory_order_acquire)) {
          if (recheckCv_.wait_for(
                  lock, std::chrono::seconds(kRecheckIntervalSeconds),
                  [this] { return stopRecheck_.load(std::memory_order_acquire); })) {
            break;
          }
          refresh();
        }
      });
    } catch (...) {
      // Thread creation fallback
    }
  }

  static constexpr int kRecheckIntervalSeconds = 5;

  NexKeyRuntimeLicenseHandle *handle_ = nullptr;
  bool configured_ = false;
  std::atomic<int> edition_{static_cast<int>(ReceiptEdition::Unknown)};
  std::atomic<int> daysRemaining_{0};
  std::atomic<unsigned long> syncCount_{0};
  std::atomic<unsigned long> manualSyncs_{0};
  std::atomic<bool> lastSyncOk_{false};
  std::atomic<int> lastSyncStatus_{-1};
  std::atomic<long long> lastSyncAt_{0};
  std::atomic<bool> syncInFlight_{false};
  std::atomic<long long> lastLocalReadAt_{0};
  std::thread syncWorker_;
  std::thread recheckWorker_;
  std::atomic<bool> stopRecheck_{false};
  std::mutex recheckMutex_;
  std::condition_variable recheckCv_;
};

#else // MC_NEXKEY_ENABLED

inline ReceiptEdition editionAtLoad() { return ReceiptEdition::Full; }

class License {
public:
  void start() {}
  void init() {}
  void shutdown() {}
  void refresh() {}
  void refreshIfStale(int = 3) {}
  void syncNow() {}
  void syncNowAsync() {}
  bool allowed() const { return true; }
  ReceiptEdition edition() const { return ReceiptEdition::Full; }
  int daysRemaining() const { return 0; }
  bool demoWatermark() const { return false; }
  bool demoChart() const { return false; }

  LicenseReport report() const {
    LicenseReport out;
    out.allowed = true;
    out.enforcing = false;
    out.status = "not compiled in";
    return out;
  }
};

#endif // MC_NEXKEY_ENABLED

} // namespace mc

#endif // MC_LICENSE_H
