# Examples

Each directory is one integration, written to be read in order and copied
from. None of them builds standalone in this repository: there is no binary
Release yet, so there is nothing to link against. See
[cmake-consumer](cmake-consumer) for how consumption is meant to look once
there is.

| | What it shows | Read alongside |
|---|---|---|
| [minimal](minimal) | **Profile A** — the host application activates, your plugin only verifies. The common case for plugins. | [INTEGRATION.md](../docs/INTEGRATION.md) |
| [activation](activation) | **Profile B** — your product collects the key, activates, and releases the seat itself. | [INTEGRATION.md](../docs/INTEGRATION.md) |
| [updates](updates) | Release checks and product notices — the second handle, independent of licensing. | [UPDATES_AND_NOTICES.md](../docs/UPDATES_AND_NOTICES.md) |
| [offline](offline) | Activating a machine with no network access, and giving its seat back. | [OFFLINE.md](../docs/OFFLINE.md) |
| [cmake-consumer](cmake-consumer) | How to find and link the SDK from your own CMake project. | |

## Two things worth taking from them

**The render callback rule.** `nexkeyruntime_license_render_decision` is the
only call designed for a render callback — one atomic read, no I/O, no lock.
Everything else belongs on a load or UI thread. `minimal` shows the shape.

**The update handle does not deduplicate itself.** One per process, not one
per plugin instance, or fifty instances will issue fifty requests for the
same answer. The license handle does share its background refresh; do not
carry that assumption over. `updates` shows the singleton.
