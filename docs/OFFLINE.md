# Offline activation

For machines that cannot reach the network — an air-gapped installation, a
facility that does not allow outbound connections, or a backend that is
unreachable when the user needs to work.

It is also the mechanism behind Nexus's continuity commitment: because a
certificate is verified against the keyring inside your product and not
against a server, this same exchange is what lets a developer keep customers
running with no Nexus infrastructure involved at all. See
[Continuity and Recovery](https://github.com/ciqueira/MCNexus/blob/main/docs/CONTINUITY.md).

The exchange is three files and one round trip by whatever means the user
has: a shared folder, a USB stick, an email to support.

```text
    machine that needs a licence            whoever administers it
    ────────────────────────────            ──────────────────────
    export_activation_request(path)  ──►    request file
                                            (issues a certificate)
    publish_receipt(certificate)     ◄──    certificate

    ... later, to give the seat back ...

    export_deactivation_proof(path)  ──►    proof file
                                            (releases the seat)
```

## Asking for a licence

```c
nexkeyruntime_license_set_license_key(g_license, user_entered_key); /* optional */
nexkeyruntime_license_export_activation_request(g_license, "/path/to/request.json");
```

Writes a small JSON file identifying this machine: which tenant, which
entitlement, this machine's binding, and a short prefix of the license key if
one has been entered. **It contains no secret.** The binding is derived from
the machine, not the machine's hardware identifiers themselves, and the file
is not signed — this machine holds no key to sign with.

The license key is optional here, unlike online activation. Exporting a
request for a machine where the user has not typed a key yet is a real case:
the operator often collects it separately.

Requires `set_product_data` (or `set_product_file`) and `set_tenant_id` to
have succeeded first — the same precondition as `load_local`. Overwrites the
file if it exists.

## Installing what comes back

```c
/* Read the certificate the administrator sent back, then: */
nexkeyruntime_license_publish_receipt(g_license, certificate_text);
```

The certificate is a plain signed document, not tied to a particular filename.
It is verified against the ProductData keyring exactly as any other receipt
is — on this call, and on every load afterwards. Importing it offline weakens
nothing: a certificate issued for another machine, or altered in transit, is
rejected here for the same reasons it would be rejected online.

On success the receipt is on disk and `render_decision` answers `ALLOW`.

## Giving the seat back

```c
nexkeyruntime_license_export_deactivation_proof(g_license, "/path/to/proof.json");
```

This stops the licence working on this machine **and then** writes the proof.
The order is deliberate and not configurable: the local receipts are deleted
first, so by the time the proof exists the machine has already stopped
rendering.

That ordering is what makes the file trustworthy without a signature. Forging
it gains the forger nothing, because their own copy stopped working before
the file existed. Whoever receives it releases the seat on that basis.

Returns `NEXKEYRUNTIME_E_NO_RECEIPT` when there was nothing to give up.

**It is not reversible locally.** Getting the machine working again means a
fresh request/certificate round trip.

## What to tell your users

Offline licences still expire, and they still carry an offline window. A
machine that never reaches the network is relying entirely on the validity
baked into its certificate, so the practical question for an air-gapped site
is how long that window is — arrange it with your vendor before the machine
goes offline, not after.
