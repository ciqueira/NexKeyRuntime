# Updates and notices

The second handle, `NexKeyRuntimeHandle`, answers two questions for your
product: **is there a newer release?** and **is there anything the vendor
needs to tell this user?**

It is completely independent of the license handle. A product can use one,
the other, or both, and neither knows the other exists. There is no license
key here, no receipt, and no activation: release and notice information is
public by design, so nothing needs to be signed or kept secret.

## What it does

- Asks the backend what the newest release is for your product, on your
  platform, architecture and channel, and tells you whether it is newer than
  what is running.
- Delivers notices the vendor published — compatibility warnings, security
  advisories, maintenance windows, migration and deprecation notes — already
  filtered to the audience they were meant for, and already resolved to the
  user's language.
- Opens the right place in MCNexus when the user acts on either, falling back
  to a web URL when MCNexus is not installed.
- Schedules its own checks so you do not have to.

## Before you start

`tenant_id`, `artifact_id` and the `base_url` for your product are issued
together with your ProductData blob, as part of per-project setup — see the
[README](../README.md). They are not discoverable at runtime and there is no
self-service signup yet.

## One handle per process, not per instance

**This handle does not deduplicate itself.** Every handle you create checks on
its own and issues its own requests. That is the opposite of the license
handle, which shares one background refresh across every instance in the
process — do not carry that assumption over.

For a plugin, this matters: a project with fifty instances of your effect must
not create fifty handles, or you will issue fifty requests for the same
answer. Create one for the whole plug-in — a process-wide singleton created on
first use — and let every instance read from it.

## Lifecycle

### 1. Create

```c
NexKeyRuntimeConfig config;
nexkeyruntime_config_init(&config);          /* fills in the defaults */

config.tenant_id       = "your-tenant-id";
config.artifact_id     = "default";          /* your product within the tenant */
config.current_version = "1.4.2";            /* what is running right now */
config.platform        = "macos";            /* "macos" or "windows" */
config.architecture    = "arm64";            /* "arm64" or "x86_64" */
config.channel         = "stable";           /* "stable", "beta" or "nightly" */
config.locale          = "pt-BR";            /* preferred language for notices */
config.base_url        = "https://your-endpoint";

/* Optional, and how notices reach the right hosts — see Audience below. */
config.host_name    = "DaVinciResolve";
config.host_version = "20.0.1";

/* Required for the open_* calls to do anything. Return non-zero when the
   URL actually opened: that is how the SDK knows to fall back from the
   mcnexus:// deep link to the https:// one. */
config.can_open_url = my_can_open_url;
config.open_url     = my_open_url;
config.user_data    = self;

NexKeyRuntimeHandle *updates = nexkeyruntime_create(&config);
if (!updates) {
    /* One of the values above was rejected. tenant_id, current_version,
       platform and architecture have no defaults and must be set; versions
       must parse; base_url must be https. */
}
```

`nexkeyruntime_config_init` fills in `artifact_id`, `channel`, `locale`,
`base_url` and the request timeout. **Set `base_url` to the endpoint issued
for your tenant** rather than relying on the built-in default.

### 2. Check

```c
nexkeyruntime_request_check(updates, 0);   /* 0 = honour the schedule */
```

**This returns immediately and does the work in the background.** It is safe
to call from a UI thread and must never be called from a render callback.

The backend tells the SDK how often to check, and `force = 0` respects that:
calling it more often than the schedule allows simply returns without going
anywhere. Pass `force = 1` only when the user explicitly asked — a "check for
updates" button — because it bypasses the schedule.

There is no completion callback, and the SDK never writes to your UI. Read
the result at a moment that suits your host: a timer, a window becoming
active, or — where the host offers neither, as in an audio or video plug-in
API — the next time the user interacts with your controls. Checking again
costs nothing when the schedule says it is too soon, so calling it on every
interaction is a reasonable pattern.

A check that is still running reports `CHECKING`, and the previous answer
stays valid until the new one lands. Nothing is lost by reading at an
inconvenient moment.

The practical consequence: on a cold install, the first time a user opens
your UI there may be no answer yet. Show nothing rather than an error — the
answer will be there the next time they look.

### 3. Read the result

```c
NexKeyRuntimeUpdateSnapshot snapshot;
snapshot.struct_size = sizeof(snapshot);
if (nexkeyruntime_get_snapshot(updates, &snapshot) == NEXKEYRUNTIME_OK) {
    /* snapshot.status:  IDLE, CHECKING, UP_TO_DATE, AVAILABLE,
                         OFFLINE, ERROR                                  */
    if (snapshot.has_update) {
        /* snapshot.latest_version, .release_id, .notes_url               */
    }
}

NexKeyRuntimeNotice notice;
notice.struct_size = sizeof(notice);
if (nexkeyruntime_get_active_notice(updates, &notice) == NEXKEYRUNTIME_OK &&
    notice.available) {
    /* notice.title and .message are already in the user's language.
       notice.type and .severity say what kind and how urgent.           */
}
```

`get_active_notice` returns `NEXKEYRUNTIME_NOT_AVAILABLE` when there is
nothing to show. That is the normal case, not an error.

Both reads are safe from any thread and never block on the network — they
report whatever the last completed check produced. Neither belongs in a
render callback, though: for that, the license handle's `render_decision` is
the only call designed for the hot path.

Set `struct_size` before every call, as in the examples. It is how the SDK
stays compatible when a future version adds a field.

### 4. Act

```c
nexkeyruntime_open_update_action(updates);          /* the release        */
nexkeyruntime_open_notice_action(updates, notice.id); /* that notice      */
```

Both open MCNexus at the relevant place, falling back to a web page when it
is not installed. This is the only external action the SDK offers — it never
downloads or installs anything itself.

### 5. Destroy

```c
nexkeyruntime_destroy(updates);
```

Returns only once any check in flight has been cancelled and finished, so a
host may unload your bundle immediately afterwards. If you followed the
singleton advice above, destroy it when your plug-in unloads, not when an
instance closes.

## Two channels, not one

Updates and notices are **independent**. `get_snapshot` answers about the
release; `get_active_notice` answers about notices; the SDK never ranks one
against the other.

If your UI shows one item at a time, you decide the order. The recommended
one:

```text
critical notice
recommended notice
update
info notice
```

**The part that is easy to get wrong:** only `recommended` and `critical`
should outrank an available update. Letting any notice win means an
informational message can hide a release the user wants, and nothing will
report that as a problem.

## Notices

### Audience

A notice reaches a user only if it applies to them. The vendor scopes each one
by product version range, platform, architecture, channel, and optionally by
host application and host version. The SDK evaluates all of it locally — the
host you are running in is never sent to the backend.

This is why `host_name` and `host_version` are worth setting: without them, a
notice scoped to a specific host matches nobody. Pass exactly what your host
reports, and do not translate or normalise it — the vendor writes the rule
against that same string.

### Severity and type

**`severity` decides what gets shown.** With several notices eligible, the
most severe wins. It is also the field to drive visual weight.

**`type` is descriptive.** `update`, `compatibility`, `security`,
`maintenance`, `migration`, `deprecation` — it tells you and the user what a
notice is about, and is there for your presentation choices, such as picking
an icon or the wording of a button. It does not change what the SDK selects
or when.

### Dismiss and remind

```c
nexkeyruntime_dismiss_notice(updates, notice.id, notice.revision);
nexkeyruntime_remind_notice_later(updates, notice.id);
```

`dismiss` takes the revision as well as the id, and that is deliberate: if the
vendor corrects a notice and republishes it with a higher revision, it comes
back. A correction is not something a previous dismissal should silence.

`remind_notice_later` hides the notice for a period the vendor set on it. Both
calls return `NEXKEYRUNTIME_NOT_AVAILABLE` if the notice does not allow that
action — whether a notice can be dismissed or snoozed is the vendor's choice,
not the user's.

**Both last for the lifetime of the handle.** If your product should remember
a dismissal across restarts, persist it yourself and skip the notice on the
way to your UI. This matters most in plugins, where the host may create and
destroy your instance many times in one session.

Only one notice is presented at a time. Dismissing or snoozing the current one
lets the next eligible one through.

## Serving the manifest yourself

`base_url` points wherever you say, and the SDK asks it for:

```text
GET {base_url}/v1/tenants/{tenant_id}/manifest
      ?artifact={artifact_id}&platform={platform}
      &architecture={architecture}&channel={channel}
```

sending `Accept-Language` from your `locale`, and `If-None-Match` once it has
seen a version — answer `304` and it keeps what it has. The response is the
document described by
[schemas/update-manifest.schema.json](../schemas/update-manifest.schema.json).

If you would rather not serve HTTP at all, set `fetch_manifest` in the config
and return the bytes from wherever you like — a file next to your product, an
archive inside your installer, your own client. The SDK then does no
networking of its own and only parses and evaluates what you hand it.

**One limit to know before you plan around this.** The URLs *inside* the
manifest are restricted by the SDK, not by your server: `deepLink` must begin
with `mcnexus://updates/` or `mcnexus://notices/`, and `notesURL` and
`fallbackURL` must begin with `https://mcnexus.app/`. Anything else is
rejected and the manifest is discarded whole. So you can host the document,
but the actions in it still point at MCNexus. If you need them to point
somewhere else, read the release and notice fields from the snapshot and open
your own URLs instead of calling `open_update_action` /
`open_notice_action`.

## No network of your own

The SDK performs its own HTTPS requests through the platform's native stack.
You do not need to supply an HTTP client, and there is nothing to configure
beyond `base_url` and the timeout.
