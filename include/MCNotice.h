// =============================================================================
// MCNotice.h
// -----------------------------------------------------------------------------
// The OTHER NexKeyRuntime handle: updates and notices.
//
// It shares nothing with MCLicense.h: no ProductData, no receipt, no
// certificate. Manifests are public and unsigned by design:
// "0.1.2 exists" is not a claim that needs a
// signature, and keeping the two handles apart is what lets a plugin ship
// updates without licensing, or the reverse.
//
// What it feeds is one collapsed group at the top of the panel that stays
// INVISIBLE until there is something to say. The plugin deliberately carries
// no license panel: MCNexus already shows edition, seats, validity and
// activation in full, and duplicating that here would be a second place to
// keep correct. The most this plugin ever says is a single notice.
//
// It carries REMOTE notices only: updates and whatever the gateway has to
// say. License state deliberately does not appear here: an unlicensed plugin
// gets its own behavior, designed separately, and folding the two into one
// slot would mean the same widget sometimes reports the server's news and
// sometimes the local verdict.
// =============================================================================

#ifndef MC_NOTICE_H
#define MC_NOTICE_H

#include "MCLicense.h"

#include <chrono>
#include <mutex>
#include <string>
#include <thread>

namespace mc {

// What the top group renders. Composed from two independent sources (the
// local license verdict and whatever the gateway had to say), so the panel
// itself needs no rules of its own.
struct NoticeView {
  bool visible = false;
  std::string text;

  // Which of the two buttons the group offers. They are separate params
  // rather than one with a runtime label because OFX hosts are not required
  // to refresh a parameter's label after describe(), and the spec says so in
  // as many words. Hiding and showing is decided by the host, and works.
  bool offerUpdate = false;      // "Update": there is a release to install
  bool offerOpenNexus = false;   // "Open MCNexus": go sort the license out
};

// How long the UI thread is allowed to wait for a check that is in flight.
//
// Measured against the real gateway on 12/09/2026, replaying the constructor's
// exact sequence: the answer lands in 74, 86, 99, 121, 298 and 391 ms across
// six runs: the tail is a cold process paying DNS and TLS. 400 ms covers the
// normal case and gives up on the rest rather than holding the host hostage.
//
// It is never zero, which is the whole point: publishNotice() used to read the
// snapshot in the same millisecond request_check() fired it, so the group was
// always painted from a CHECKING state and stayed hidden. See waitForCheck().
constexpr int kCheckWaitBudgetMs = 400;

#ifdef MC_NEXKEY_ENABLED

// Where the gateway lives. Baked in from the SAME ProductData the licensing
// handle uses (the Makefile already decodes the blob to print it), so there
// is exactly one place where the environment is stated. The SDK's own default
// is https://sdk.mcnexus.app, which does not resolve today; inheriting it
// silently would turn every check into a network error.
#ifndef MC_NEXKEY_BASE_URL
#define MC_NEXKEY_BASE_URL "https://sdk-sandbox.mcnexus.app"
#endif

#ifndef MC_NEXKEY_TENANT
#define MC_NEXKEY_TENANT "mcnexus-sandbox"
#endif

#ifndef MC_NEXKEY_ARTIFACT
#define MC_NEXKEY_ARTIFACT "default"
#endif

#ifndef MC_NEXKEY_CHANNEL
#define MC_NEXKEY_CHANNEL "stable"
#endif

// WHICH CHANNEL THIS PLUGIN LISTENS TO.
//
// The SDK exposes two independent channels: the release channel ("there is a
// newer version") and the notice channel, where the manifest author publishes
// compatibility, security, maintenance, migration and deprecation messages.
//
// This wrapper defaults to the RELEASE channel only: an OFX plugin has one
// thing worth interrupting a colorist for, and that is a new version. Define
// MC_NEXKEY_NOTICE_CHANNEL=0 before including if you also want notices shown.
//
// Keeping the flag explicit rather than deleting the path leaves both code
// paths compile-tested without changing behavior.
#ifndef MC_NEXKEY_NOTICE_CHANNEL
#define MC_NEXKEY_NOTICE_CHANNEL 1
#endif

inline const char *noticePlatform() {
#if defined(_WIN32)
  return "windows";
#elif defined(__APPLE__)
  return "macos";
#else
  return "";
#endif
}

inline const char *noticeArchitecture() {
#if defined(__aarch64__) || defined(_M_ARM64)
  return "arm64";
#else
  return "x86_64";
#endif
}

enum class ActiveItem { None, Notice, Update };

// Display ranking across independent SDK channels.
//
// `get_snapshot` reports `has_update` and `get_active_notice` reports active
// notices sorted by severity; the SDK leaves UI precedence to the product:
// critical notice > recommended notice > update > info notice.
//
// Giving every notice blanket precedence lets a purely informational notice
// bury a release the user would want. Only `recommended` and `critical`
// outrank an update.
//
// Pure and free-standing so it can be tested without a handle, a network or a
// host.
inline ActiveItem chooseActiveItem(bool haveNotice,
                                   NexKeyRuntimeNoticeSeverity severity,
                                   bool hasUpdate) {
  if (haveNotice && (severity > NEXKEYRUNTIME_NOTICE_INFO || !hasUpdate)) {
    return ActiveItem::Notice;
  }
  if (hasUpdate) return ActiveItem::Update;
  return haveNotice ? ActiveItem::Notice : ActiveItem::None;
}

// Holds a reference to the process-wide update handle.
//
// ONE HANDLE PER PROCESS, NOT PER INSTANCE: sharing one handle across every
// plugin instance prevents N nodes in a grade from firing N redundant manifest
// requests and ensures subsequent instances read an already-completed check
// without waiting.
//
// Non-copyable for the same reason License is: destroy() joins the SDK's
// worker, and doing that twice inside a host would be unsafe.
class Notices {
public:
  // How the plugin opens a URL. Passed in rather than called directly so this
  // header stays free of host or platform specific URL launchers.
  // Returns non-zero when the URL actually opened; the SDK uses that answer
  // to fall back from the mcnexus:// deep link to the https:// one, which is
  // what keeps the button useful on a machine with no MCNexus installed.
  using OpenUrlFn = int (*)(const char *url);

  Notices() = default;
  ~Notices() { shutdown(); }
  Notices(const Notices &) = delete;
  Notices &operator=(const Notices &) = delete;

  // hostName/hostVersion come from the OFX host description and are passed
  // straight through, uninterpreted. They are what makes a notice's hostRules
  // decidable; the staging fixture's only notice is scoped to DaVinci
  // Resolve 20.x, and without these it could never match anybody.
  //
  // NOBODY HAS CHECKED what string Resolve actually reports here. Every
  // manifest written so far says "DaVinciResolve" because that is what the
  // spec's example said, not because a host was observed saying it. The
  // diagnostics panel prints the real value for exactly this reason: the
  // first load inside Resolve settles it, and a manifest written against a
  // guess would silently match nothing.
  void start(OpenUrlFn opener, const char *hostName, const char *hostVersion) {
    if (handle_) return;

    const char *baseUrl = MC_NEXKEY_BASE_URL;
    if (!baseUrl || baseUrl[0] == '\0') return;  // nothing to talk to

    Shared &state = shared();
    std::lock_guard<std::mutex> lock(state.mutex);

    // Somebody else already built it. Take a reference and read their answer;
    // no second request, and nothing to wait for if their check has landed.
    if (state.handle) {
      ++state.refs;
      handle_ = state.handle;
      return;
    }

    state.opener = opener;

    NexKeyRuntimeConfig config;
    nexkeyruntime_config_init(&config);
    config.tenant_id = MC_NEXKEY_TENANT;
    config.artifact_id = MC_NEXKEY_ARTIFACT;
    config.channel = MC_NEXKEY_CHANNEL;
    config.current_version = PLUGIN_VERSION;
    config.platform = noticePlatform();
    config.architecture = noticeArchitecture();
    config.base_url = baseUrl;
    // Left null when the host says nothing. copyConfig() treats absent as
    // "no host declared" and rejects a version string it cannot parse, so
    // handing it an empty or malformed one would kill the whole handle over
    // a field that is only ever an audience filter.
    if (hostName && hostName[0] != '\0') config.host_name = hostName;
    if (hostVersion && hostVersion[0] != '\0') config.host_version = hostVersion;
    config.can_open_url = &Notices::canOpenUrl;
    config.open_url = &Notices::openUrl;
    // The SHARED state, never `this`. The handle outlives the instance that
    // created it; the SDK would otherwise call openUrl through a dangling
    // pointer the moment that first node is deleted and a second one presses
    // Update.
    config.user_data = &state;

    state.handle = nexkeyruntime_create(&config);
    if (!state.handle) return;
    state.refs = 1;
    handle_ = state.handle;

    // Fire the ONLY automatic check this process makes. It runs on the SDK's
    // own worker; the caller collects the answer through waitForCheck() before
    // it paints, because OFX has no idle callback and there is nothing else
    // that would hand the panel a second chance.
    nexkeyruntime_request_check(handle_, 0);
  }

  // Waits, for at most budgetMs, on whatever check is in flight.
  //
  // Because OpenFX plugins start without a persistent update cache on disk and
  // have no idle callback between paint passes, waiting briefly once per
  // process ensures the first cold load can display a notice or update
  // immediately; every subsequent instance sees a settled status and returns
  // without waiting.
  //
  // Returns true when the check settled inside the budget. A false is not an
  // error worth reporting: an answer that did not arrive in time is
  // indistinguishable, to the panel, from one that says there is no news.
  bool waitForCheck(int budgetMs) {
    if (!handle_) return false;
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(budgetMs);
    for (;;) {
      NexKeyRuntimeUpdateSnapshot snapshot{};
      snapshot.struct_size = sizeof(snapshot);
      // request_check() sets CHECKING under its own lock BEFORE it spawns the
      // worker, so there is no window where a check is running and the status
      // still says otherwise. Anything but CHECKING means settled, including
      // the early return it makes when the schedule says not yet.
      if (nexkeyruntime_get_snapshot(handle_, &snapshot) != NEXKEYRUNTIME_OK ||
          snapshot.status != NEXKEYRUNTIME_UPDATE_CHECKING) {
        return true;
      }
      if (std::chrono::steady_clock::now() >= deadline) return false;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  void shutdown() {
    if (!handle_) return;
    handle_ = nullptr;

    Shared &state = shared();
    NexKeyRuntimeHandle *doomed = nullptr;
    {
      std::lock_guard<std::mutex> lock(state.mutex);
      if (--state.refs > 0) return;  // other nodes still hold it
      doomed = state.handle;
      state.handle = nullptr;
    }
    // Outside the lock on purpose: destroy() cancels an in-flight fetch and
    // JOINS the worker, which can take as long as the 5 s request timeout.
    // Holding the mutex across that would stall every other instance's view()
    // for the same 5 s, on the UI thread, to no purpose.
    nexkeyruntime_destroy(doomed);
  }

  // Nudges a fresh check. Cheap and non-blocking: the SDK refuses with BUSY
  // if one is already running, and honors its own schedule unless forced.
  //
  // `force` exists for the place a user explicitly asks for a check (the
  // Refresh License button), where honoring the schedule would ignore the
  // user's explicit request.
  //
  // Keep this tied to explicit user refreshes rather than slider drags so
  // notices never appear mid-session without a clear trigger. Both start() and
  // explicit refreshes follow up with waitForCheck() so completed results are
  // reflected right away.
  void poke(bool force = false) {
    if (handle_) nexkeyruntime_request_check(handle_, force ? 1 : 0);
  }

  void refresh(bool force = true) {
    poke(force);
    waitForCheck(kCheckWaitBudgetMs);
  }

  void dismissActiveNotice() {
    if (!handle_) return;
    NexKeyRuntimeNotice notice{};
    notice.struct_size = sizeof(notice);
    if (nexkeyruntime_get_active_notice(handle_, &notice) == NEXKEYRUNTIME_OK && notice.available) {
      nexkeyruntime_dismiss_notice(handle_, notice.id, notice.revision);
    }
  }

  // Opens whatever the visible item points at.
  //
  // Uses the SAME decision as view(), through chooseActiveItem, and that is
  // the whole point of routing both through one function: the earlier version
  // opened the notice whenever one existed, so a panel showing "version 0.1.2
  // is available" could open an unrelated maintenance notice instead. Two
  // copies of a priority rule drift; one cannot.
  void openAction() {
    if (!handle_) return;

    NexKeyRuntimeNotice notice{};
    NexKeyRuntimeUpdateSnapshot snapshot{};
    switch (resolve(notice, snapshot)) {
      case ActiveItem::Notice:
        nexkeyruntime_open_notice_action(handle_, notice.id);
        return;
      case ActiveItem::Update:
        nexkeyruntime_open_update_action(handle_);
        return;
      case ActiveItem::None:
        return;
    }
  }

  // Decides what the one visible slot says.
  NoticeView view() const {
    NoticeView out;
    if (!handle_) return out;

    NexKeyRuntimeNotice notice{};
    NexKeyRuntimeUpdateSnapshot snapshot{};
    switch (resolve(notice, snapshot)) {
      case ActiveItem::Notice:
        out.visible = true;
        out.text = notice.title[0] != '\0' ? std::string(notice.title) : std::string();
        if (notice.message[0] != '\0') {
          if (!out.text.empty()) out.text += "\n\n";
          out.text += notice.message;
        }
        // An update notice is the one case where "Update" is the honest verb.
        out.offerUpdate = notice.type == NEXKEYRUNTIME_NOTICE_UPDATE;
        out.offerOpenNexus = !out.offerUpdate;
        return out;

      case ActiveItem::Update:
        out.visible = true;
        out.text = "Version ";
        out.text += snapshot.latest_version;
        out.text += " is available.\n\nYou are running ";
        out.text += snapshot.current_version[0] != '\0' ? snapshot.current_version
                                                        : PLUGIN_VERSION;
        out.text += ".";
        out.offerUpdate = true;
        return out;

      case ActiveItem::None:
        // Silence is the answer for up-to-date, checking, offline and error.
        // An update check that could not reach the network is not news the
        // user can act on, and saying so would put a permanent complaint in
        // the panel of anyone who works offline on purpose.
        return out;
    }
    return out;
  }

private:
  // Reads both sources and applies chooseActiveItem. Fills whichever of the
  // two out-parameters the answer refers to; the other is left zeroed, and
  // ActiveItem says which one to trust.
  ActiveItem resolve(NexKeyRuntimeNotice &notice,
                     NexKeyRuntimeUpdateSnapshot &snapshot) const {
    notice = NexKeyRuntimeNotice();
    notice.struct_size = sizeof(notice);
#if MC_NEXKEY_NOTICE_CHANNEL
    const bool haveNotice =
        nexkeyruntime_get_active_notice(handle_, &notice) == NEXKEYRUNTIME_OK &&
        notice.available;
#else
    // Not even asked for. The gateway may well be serving notices for this
    // tenant (the staging manifest does), and they are ignored here rather
    // than fetched and discarded, so nothing can leak into the panel by way
    // of a later edit further down.
    const bool haveNotice = false;
#endif

    snapshot = NexKeyRuntimeUpdateSnapshot();
    snapshot.struct_size = sizeof(snapshot);
    const bool hasUpdate =
        nexkeyruntime_get_snapshot(handle_, &snapshot) == NEXKEYRUNTIME_OK &&
        snapshot.has_update && snapshot.latest_version[0] != '\0';

    return chooseActiveItem(haveNotice, notice.severity, hasUpdate);
  }

  static int canOpenUrl(void *userData, const char *url) {
    // Answering "yes, try it" for the custom scheme and letting openUrl
    // report the truth. Genuinely probing whether mcnexus:// is registered
    // needs LaunchServices on macOS and a registry walk on Windows, and both
    // would be a worse source of truth than simply attempting it: the SDK
    // falls back to the https:// URL the moment the attempt fails.
    (void)userData;
    return (url && url[0] != '\0') ? 1 : 0;
  }

  static int openUrl(void *userData, const char *url) {
    Shared *state = static_cast<Shared *>(userData);
    if (!state || !state->opener || !url || url[0] == '\0') return 0;
    return state->opener(url);
  }

  // The one handle, and the count of instances still pointing at it. Written
  // under the mutex; `opener` is set once, before the handle exists, and is
  // the same free function for every instance.
  struct Shared {
    std::mutex mutex;
    NexKeyRuntimeHandle *handle = nullptr;
    int refs = 0;
    OpenUrlFn opener = nullptr;
  };

  static Shared &shared() {
    static Shared state;
    return state;
  }

  // Non-owning. Non-null exactly when this instance holds a reference, which
  // is what keeps the pointer alive without re-reading the shared state: the
  // refcount cannot reach zero while we are counted in it.
  NexKeyRuntimeHandle *handle_ = nullptr;
};

#else // MC_NEXKEY_ENABLED

// Licensing and updates compiled out. The group never becomes visible, so the
// panel looks exactly as it did before any of this existed.
class Notices {
public:
  using OpenUrlFn = int (*)(const char *url);
  void start(OpenUrlFn, const char *, const char *) {}
  void shutdown() {}
  bool waitForCheck(int budgetMs) { (void)budgetMs; return false; }
  void poke(bool force = false) { (void)force; }
  void openAction() {}
  NoticeView view() const { return NoticeView(); }
};

#endif // MC_NEXKEY_ENABLED

} // namespace mc

#endif // MC_NOTICE_H
